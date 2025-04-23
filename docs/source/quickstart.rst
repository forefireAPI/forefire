Quick Start
===========
**Try ForeFire fast with Docker**

This guide shows the quickest way to see ForeFire in action using its interactive web console, powered by Docker. This method bundles all dependencies, so you don't need to install them on your host system.

Prerequisites
-------------
- Docker installed and running on your system.
- Git installed (for cloning the repository).

Steps
-----

1.  **Clone the ForeFire repository:** Open your terminal and run:

  .. code-block:: bash

    git clone https://github.com/forefireAPI/firefront.git

2.  **Build the Docker image:**

  Navigate into the cloned repository directory. The `Dockerfile` defines the environment. Build the image with:
  
  .. code-block:: bash

    cd firefront
    docker build . -t forefire:latest

  This might take a few minutes the first time as it downloads base images and installs dependencies inside the Docker build environment.

3.  **Run the container interactively:**

  This command starts the container, maps port 8000 for the web interface, and gives you a bash shell inside the container:
  
  .. code-block:: bash

    docker run -it --rm -p 8000:8000 --name ff_interactive forefire bash

4.  **Inside the container, navigate to the test directory:**

  Your terminal prompt should now show you are inside the container (e.g., `root@<container_id>:/app#`). Navigate to the test directory:

  .. code-block:: bash

    cd tests/runff

5.  **Start the ForeFire interpreter:**

  .. code-block:: bash

    forefire

6.  **Launch the HTTP server from the ForeFire console:**

  Type the following command at the `forefire>` prompt:

  .. code-block:: none

    forefire> listenHTTP[]

  You should see output like: `>> HTTP command server listening on port 8000`

7.  **Access the Web Console:**

  Open your local web browser (on your host machine, not inside the container) and navigate to `http://localhost:8000/`.

8.  **Run a Simulation:**

  *   In the web console's command input box, type: `include[real_case.ff]` and press Enter or click Send.
  *   Click the "Refresh Map" button.

  You should see a simulation running in the Aullène region of Corsica. You can explore other commands or modify parameters via the web console.

9.  **Stop the Container:**

  When finished exploring:

  - In the ForeFire console (either web or terminal inside the container), type `quit`.
  - In the container's bash shell (terminal), type `exit`.
  - The `docker run` command used `--rm`, so the container will be automatically removed upon exit.
