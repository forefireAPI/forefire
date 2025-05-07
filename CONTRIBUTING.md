# Contributing to ForeFire

First off, thank you for considering contributing to ForeFire! We welcome contributions from the community, whether it's reporting bugs, suggesting enhancements, improving documentation, or submitting code changes.

This document provides guidelines for contributing to the project.

## How Can I Contribute?

*   [Reporting Bugs](#reporting-bugs)
*   [Suggesting Enhancements](#suggesting-enhancements)
*   [Contributing Code](#contributing-code)
*   [Improving Documentation](#improving-documentation)

## Reporting Bugs

If you encounter a bug while using ForeFire, please help us by reporting it! Good bug reports are essential for improving the software.

1.  **Check Existing Issues:** Before submitting a new issue, please search the [GitHub Issues](https://github.com/forefireAPI/forefire/issues) to see if the bug has already been reported.
2.  **Gather Information:** If the bug hasn't been reported, please gather the following information:
    *   ForeFire version (`forefire -v`).
    *   Operating System (e.g., Ubuntu 22.04, macOS Sonoma, WSL on Windows 11).
    *   Compiler used (if built from source).
    *   A clear and concise description of the bug.
    *   Steps to reproduce the bug reliably. Include relevant parts of your `.ff` script file, input data details, or a minimal example if possible.
    *   What you expected to happen.
    *   What actually happened (include any error messages or incorrect output).
3.  **Submit the Issue:** Create a new issue on the [GitHub Issues](https://github.com/forefireAPI/forefire/issues) page, providing the information gathered above. Use a descriptive title.

## Suggesting Enhancements

We are open to suggestions for new features or improvements to existing functionality.

1.  **Check Existing Issues/Discussions:** Search the [GitHub Issues](https://github.com/forefireAPI/forefire/issues) (look for "enhancement" or "feature request" labels) and potentially [GitHub Discussions](https://github.com/forefireAPI/forefire/discussions) (if enabled) to see if your idea has already been discussed.
2.  **Submit the Suggestion:** If your idea is new, create a new issue on the [GitHub Issues](https://github.com/forefireAPI/forefire/issues) page.
    *   Use a clear and descriptive title.
    *   Explain the enhancement you would like to see.
    *   Describe the motivation or use case for the enhancement (why it would be valuable).
    *   Provide examples or details on how you envision the feature working, if applicable.

## Contributing Code

We welcome code contributions, from bug fixes to new features.

1.  **Discuss First (for major changes):**

    If you plan to implement a significant new feature or make major changes to the architecture, please open an issue first to discuss your proposal with the development team. This helps ensure your contribution aligns with the project's goals and avoids duplicated effort. For smaller bug fixes, feel free to proceed directly to a Pull Request.

2.  **Set Up Development Environment:**

    Follow the instructions in the [Installation Guide](https://forefire.readthedocs.io/en/latest/getting_started/installation.html) to build ForeFire from source. *(Update link if RTD URL changes)*

3.  **Fork the Repository:**

    Create your own fork of the `forefireAPI/forefire` repository on GitHub.

4.  **Create a Branch:**

    Create a new branch in your fork for your changes. Use a descriptive name (e.g., `fix-rothermel-bug`, `feature-add-new-model`).
    ```bash
    git checkout -b my-feature-branch
    ```
5.  **Make Changes:**

    Implement your code changes, following existing coding style and conventions where possible.
6.  **Add Tests (if applicable):**

    For new features or significant bug fixes, please add corresponding tests or update existing ones. See the [Testing Documentation](https://forefire.readthedocs.io/en/latest/developer_guide/testing.html) *(Placeholder: Link to testing section once created)* for details on how to run tests. Ensure all tests pass locally.
    ```bash
    # Example command to run tests (adjust as needed)
    cd tests && bash run.bash
    ```
7.  **Commit Changes:**

    Commit your changes with clear and concise commit messages.
    ```bash
    git add .
    git commit -m "feat: Implement new flux model calculation"
    ```
8.  **Push to Your Fork:**

    Push your branch to your GitHub fork.
    ```bash
    git push origin my-feature-branch
    ```
9.  **Open a Pull Request (PR):**

    Go to the `forefireAPI/forefire` repository on GitHub and open a Pull Request from your branch to the `dev` branch (or `master` if that's the target).
    *   Provide a clear description of the changes in the PR.
    *   Link any relevant issues (e.g., "Fixes #123").
    *   Ensure the CI checks pass on your PR. The maintainers will review your code, provide feedback, and merge it once approved.

## Improving Documentation

Good documentation is crucial! If you find errors, areas that are unclear, or sections that could be expanded, please help improve it.

*   Documentation source files are located in the `docs/source/` directory.
*   They are written in reStructuredText (`.rst`).
*   You can follow the same Fork -> Branch -> Edit -> Pull Request workflow as for code contributions to suggest changes to the documentation files.

## Code of Conduct

All contributors are expected to adhere to the project's [Code of Conduct](CODE_OF_CONDUCT.md). Please be respectful and considerate in all interactions.

Thank you for contributing to ForeFire!