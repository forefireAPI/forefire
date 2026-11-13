"""Portable format/accuracy tests: python -m unittest discover -s tools/tiles."""
import tempfile
from pathlib import Path
import unittest
import numpy as np
from fftile import HEADER, geometry, write_tile, read_tile, decoded_fields

class FFTileTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path=Path(self.temp.name)/'test.fftile'
        self.a=np.zeros((16,16),dtype=np.float32)
        self.fuel=np.zeros((32,32),dtype=np.uint8)
        self.fuel[:16]=2;self.fuel[16:]=253
    def write(self,**kw):
        arrays=dict(altitude=self.a,sx=self.a,sy=self.a,muu=self.a+1,muv=self.a,mvu=self.a,mvv=self.a+1)
        arrays.update(kw)
        return write_tile(self.path,42,8,self.fuel,**arrays,timestamp=0)
    def test_header_and_fuel_are_exact(self):
        self.write();meta,fields=decoded_fields(self.path)
        self.assertEqual(HEADER.size,256)
        np.testing.assert_array_equal(fields['fuel'],self.fuel)
        self.assertEqual(meta['created'],0)
        self.assertEqual(meta['core'],(8.,42.,8.5,42.5))
    def test_constants_have_no_offset_error(self):
        self.write(altitude=self.a-13)
        _,fields=decoded_fields(self.path)
        np.testing.assert_array_equal(fields['altitude'],self.a-13)
        np.testing.assert_array_equal(fields['muu'],self.a+1)
    def test_signed_atlas_components_remain_separate(self):
        self.write(sx=self.a-.3,sy=self.a+.4,muv=self.a-.7,mvu=self.a+.8)
        _,fields=decoded_fields(self.path)
        for k,v in [('sx',-.3),('sy',.4),('muv',-.7),('mvu',.8)]:
            np.testing.assert_allclose(fields[k],v,atol=1e-7)
    def test_large_elevation_range_uses_lossless_16_bit(self):
        altitude=np.random.default_rng(7).uniform(-100,4000,(16,16)).astype(np.float32)
        report=self.write(altitude=altitude)
        meta,fields=decoded_fields(self.path)
        self.assertEqual(meta['flags'],3)
        self.assertEqual(report['accuracy']['altitude']['codec'],'PNG16')
        self.assertLess(np.max(np.abs(fields['altitude']-altitude)),.04)
    def test_unachievable_slope_accuracy_is_rejected(self):
        a=np.random.default_rng(4).uniform(-100,100,(16,16)).astype(np.float32)
        with self.assertRaisesRegex(ValueError,'accuracy budget'):
            self.write(sx=a)
        self.assertFalse(self.path.exists())
    def test_fuel_ids_are_not_silently_clipped(self):
        self.fuel=self.fuel.astype(float);self.fuel[0,0]=256
        with self.assertRaisesRegex(ValueError,'integer IDs'):self.write()
    def test_corrupt_header_is_rejected(self):
        self.write();b=bytearray(self.path.read_bytes());b[104]^=1;self.path.write_bytes(b)
        with self.assertRaisesRegex(ValueError,'CRC'):read_tile(self.path)
    def test_truncated_payload_is_rejected(self):
        self.write();self.path.write_bytes(self.path.read_bytes()[:-1])
        with self.assertRaisesRegex(ValueError,'payload'):read_tile(self.path)
    def test_fuel_halos_share_one_lattice_even_when_continuous_widths_change(self):
        from fftile import pixel_centers
        _,a,da=geometry(60.5,8)
        _,b,db=geometry(61,8)
        self.assertNotEqual(da[3][0],db[3][0])
        xa,ya=pixel_centers(a,*da[0]);xb,yb=pixel_centers(b,*db[0])
        np.testing.assert_allclose(xa,xb,atol=1e-13,rtol=0)
        np.testing.assert_allclose(ya[-222:],yb[:222],atol=1e-13,rtol=0)
    def test_grid_handles_southern_and_polar_tiles(self):
        for lat,lon in [(-45.5,-2.5),(-90,-180),(89.5,179.5)]:
            core,bounds,dims=geometry(lat,lon)
            self.assertEqual(core[0],lon)
            self.assertGreaterEqual(bounds[1],-90)
            self.assertLessEqual(bounds[3],90)
            self.assertTrue(all(d%16==0 for pair in dims[1:] for d in pair))
        with self.assertRaises(ValueError):geometry(42.1,8)

if __name__=='__main__':unittest.main()
