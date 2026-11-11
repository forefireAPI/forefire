# Contributing to ForeFire

Thanks for helping. Bugs, ideas, docs and code are all welcome.

## Reporting a bug

Search the [issues](https://github.com/forefireAPI/forefire/issues) first.
A new issue should include:

- `forefire -v`, your OS and, if you built from source, the compiler;
- the smallest `.ff` script or Python snippet that reproduces it;
- what you expected and what happened, with the error message.

Report security problems through
[private vulnerability reporting](https://github.com/forefireAPI/forefire/security/advisories/new),
never in a public issue.

## Suggesting a feature

Open an issue saying what you need and why. For a large feature or an
architecture change, wait for a maintainer to agree before writing code.

## Contributing code

1. Build from source following the
   [installation guide](https://forefire.readthedocs.io/en/latest/getting_started/installation.html).
2. Fork the repository, branch from `dev`, and open the pull request against
   `dev`.
3. Keep each pull request small enough to review in one sitting and follow the
   style of the surrounding code.
4. Run the tests that cover your change; [TESTING.md](TESTING.md) lists every
   suite and which ones CI runs.
5. If users will notice the change, add one line under `## [Unreleased]` in
   [CHANGELOG.md](CHANGELOG.md), written for someone upgrading.
6. Keep the pull request description short: what changes and how you checked
   it. Link the issue it fixes.

Propagation and flux models and their physics are the maintainers' decision:
start those as an issue. A change that moves simulation results goes in its
own pull request, at the top of its CHANGELOG entry. Never regenerate a `.ref`
file to make a test pass.

## Documentation

The docs are reStructuredText in `docs/source/`, built with Sphinx and
published on [Read the Docs](https://forefire.readthedocs.io/). Changes go
through the same pull request flow as code.

## Code of conduct

Follow the [Code of Conduct](CODE_OF_CONDUCT.md).
