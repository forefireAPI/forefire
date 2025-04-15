<?php
/*
 * Firecast Status index.php
 * Displays the progress of the simulation phases with a classy design.
 */

// Read fire initialization information from ForeFire/Init.ff.
$fireInfo = ["timestamp" => "Not available", "location" => "Not available"];
$initFile = "ForeFire/Init.ff";
if (file_exists($initFile)) {
    $contents = file_get_contents($initFile);
    // Extract the ISO date from the FireDomain line.
    if (preg_match('/ISOdate=([\d\-T:]+Z)/', $contents, $matches)) {
        $fireInfo["timestamp"] = $matches[1];
    }
    // Extract the longitude and latitude from the startFire line.
    if (preg_match('/startFire\[lonlat=\(([-\d.]+),\s*([-\d.]+)/', $contents, $matches)) {
        $fireInfo["location"] = $matches[1] . ", " . $matches[2];
    }
}

// Scan for phase files.
// Conditioning phase: Files matching "M1.*.nc"
$conditioningFiles = glob("M1.*.nc");
$condCount = count($conditioningFiles);
$condExpected = 7;  // Adjust the expected value as needed

// Spinup phase: Files matching "RUN12.1.PRUN*.nc"
$spinupFiles = glob("RUN12.1.PRUN*.nc");
$spinupCount = count($spinupFiles);
$spinupExpected = 4; // Upper bound expected file count

// Run phase: Files matching "RUN12.2.*.nc"
$runFiles = glob("RUN12.2.*.nc");
$runCount = count($runFiles);
$runExpected = 12;

function displayProgress($phaseName, $current, $expected) {
    $percent = ($expected > 0) ? min(100, round(($current / $expected) * 100)) : 0;
    echo "<div class='progress-section'>";
    echo "<h2>$phaseName</h2>";
    echo "<progress class='progress' value='$current' max='$expected'></progress> ";
    echo "<span>$current of $expected files ($percent%)</span>";
    echo "</div>";
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Firecast Status</title>
    <style>
        body {
            background-color: #000;
            color: #fff;
            font-family: Arial, sans-serif;
            margin: 20px;
        }
        header {
            text-align: center;
            margin-bottom: 30px;
        }
        header h1 {
            color: #FF4500;
            font-size: 4em;
            margin: 0;
            letter-spacing: 2px;
        }
        header .info {
            margin-top: 10px;
            font-size: 1.2em;
        }
        header .map-link a {
            color: #FF4500;
            text-decoration: underline;
            font-size: 1.2em;
        }
        .progress-section {
            margin-bottom: 30px;
        }
        .progress-section h2 {
            margin-bottom: 5px;
        }
        progress.progress {
            width: 100%;
            height: 20px;
            margin-bottom: 5px;
            background-color: #333;
            border: none;
        }
        /* Customize the progress bar for Webkit-based browsers */
        progress.progress::-webkit-progress-bar {
            background-color: #333;
        }
        progress.progress::-webkit-progress-value {
            background-color: #FF4500;
        }
        a {
            color: #FF4500;
            text-decoration: none;
        }
        a:hover {
            text-decoration: underline;
        }
        ul {
            list-style-type: none;
            padding-left: 0;
        }
        li {
            margin-bottom: 5px;
        }
        .status {
            font-weight: bold;
        }
    </style>
</head>
<body>
    <header>
        <h1>Firecast</h1>
        <div class="info">
            <?php
                echo "Ignition timestamp: " . htmlspecialchars($fireInfo["timestamp"]) . "<br>";
                echo "Location (lon, lat): " . htmlspecialchars($fireInfo["location"]);
            ?>
        </div>
        <div class="map-link">
            <a href="images/index.html">Interactive Map</a>
        </div>
    </header>
    
    <section class="phase">
        <?php displayProgress("Conditioning Phase", $condCount, $condExpected); ?>
    </section>
    <section class="phase">
        <?php displayProgress("Spinup Phase", $spinupCount, $spinupExpected); ?>
    </section>
    <section class="phase">
        <?php displayProgress("Run Phase", $runCount, $runExpected); ?>
    </section>
    
    <section class="report">
        <h2>Report</h2>
        <?php
        $reportPath = "report/report.pdf";
        if (file_exists($reportPath)) {
            echo "<p><a href='$reportPath'>View Report</a></p>";
        } else {
            echo "<p class='status'>Report not available</p>";
        }
        ?>
    </section>
    
    <section class="kmz-kml">
        <h2>KMZ/KML Files</h2>
        <?php
        $kmzFiles = glob("images/*.kmz");
        $kmlFiles = glob("images/*.kml");
        $allFiles = array_merge($kmzFiles, $kmlFiles);
        if (count($allFiles) > 0) {
            sort($allFiles);
            echo "<ul>";
            foreach ($allFiles as $file) {
                $basename = basename($file);
                echo "<li><a href='$file'>$basename</a></li>";
            }
            echo "</ul>";
        } else {
            echo "<p class='status'>KMZ/KML files not available</p>";
        }
        ?>
    </section>
    
</body>
</html>