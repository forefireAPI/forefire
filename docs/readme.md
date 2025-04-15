# Documentation with Read the Docs

## Generate the docs locally
Python and sphinx
- Create a venv with `python3 -m venv venv` and activate it with `venv/bin/activate`
- Install dependencies with `pip3 install -r requirements.txt`
this also might be necessary
- `sudo apt-get install python3-sphinx`

Doxygen
- install doxygen with `sudo apt-get install doxygen`
- Acess the doxygen folder, it contains the `Doxyfile` which holds the configuration. To convert the CPP code into XML run `doxygen Doxyfile` or simply `doxygen`

To build localy
- Go back to docs folder and run `make html`

## Deployment
- Deployment needs to be configured via the GUI of readthedocs
- The file `.readthedocs.yaml` contains the necessary configuration

## References
- https://docs.readthedocs.com/platform/stable/tutorial/index.html
- https://www.youtube.com/watch?v=pnnKzkNTo4w
- https://www.youtube.com/watch?v=PO4Zd-6a6fA
