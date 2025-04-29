---
# Batti this is the example provided on official JOSS DOCS
# Gemini made this draft for us having all the code base and docs
# https://joss.readthedocs.io/en/latest/example_paper.html

title: 'ForeFire: An Open-Source C++ Engine for High-Performance Wildfire Simulation' # Or similar descriptive title
tags:
  - C++
  - Python
  - wildfire simulation
  - fire behavior modeling
  - high-performance computing
  - fire-atmosphere coupling
  - numerical modeling
authors:
  - name: Jean-Baptiste Filippi
    orcid: XXXX-XXXX-XXXX-XXXX # TODO: Add J-B's ORCID
    affiliation: 1
    corresponding: true
  # - name: Other Author Name # TODO: Add other major contributors if applicable
  #   orcid: XXXX-XXXX-XXXX-XXXX
  #   affiliation: X
affiliations:
 - name: SPE, UMR 6134, CNRS, University of Corsica Pascal Paoli, Corte, France # TODO: Verify exact affiliation wording and add ROR if available
   index: 1
   # ror: 03wt46y18 # Example ROR for University of Corsica
date: DD Month YYYY # This will be set by JOSS upon acceptance
bibliography: paper.bib

# Optional fields for AAS journals (remove if not applicable)
# aas-doi:
# aas-journal:
---

# Summary

<!--
Instructions for this section:
- Write a short summary (approx. 1-2 paragraphs) describing the high-level functionality and purpose of ForeFire.
- Target audience: A diverse, non-specialist reader from any scientific/research background.
- AVOID jargon specific to wildfire modeling here. Focus on what the software *does* in general terms.
- Example points: Simulates wildfire spread, uses C++ for performance, takes geospatial data, models fire physics, can couple with weather models, has different interfaces (interpreter, library, python), used for research/forecasting.
-->

*TODO: J-B Filippi to write the Summary section.*

<!-- ForeFire is an open-source simulation engine written in C++ designed for modeling wildland fire behavior. It allows researchers and operational users to simulate fire spread across complex landscapes using various physical models. The software takes geospatial inputs (like terrain, fuel types, and weather data) and computes the fire's progression over time. Key features include a high-performance parallel core suitable for large-scale simulations, the capability to couple with atmospheric models for studying fire-weather interactions, and multiple interfaces including a command-line interpreter, a C++ library, and Python bindings. ForeFire serves as a tool for wildfire research, risk assessment, and potentially operational forecasting support. -->

# Statement of need

<!--
Instructions for this section:
- Clearly state the research problem ForeFire addresses (e.g., need for accurate/fast wildfire prediction, understanding complex fire behaviors like coupling).
- Place ForeFire in the context of existing software. Briefly mention limitations of other tools or gaps that ForeFire fills (e.g., performance on large scales, specific coupling features, open-source C++ core for extensibility). Cite 1-2 key alternatives (e.g., FARSITE, WRF-Fire).
- Highlight ForeFire's unique contributions or advantages (e.g., MPI parallelism, specific coupling design, model flexibility, open-source nature).
- Mention the intended audience/research applications.
- This section should closely align with the 'Statement of Need' added to the main documentation but be tailored for the paper format.
- Reference relevant papers using [@citekey] format, corresponding to entries in paper.bib.
-->

*TODO: J-B Filippi to write the Statement of Need section, citing relevant literature and alternative software.*

<!-- Wildfire modeling is critical for understanding fire dynamics, assessing risk, and supporting operational decisions. While several tools exist (e.g., FARSITE [@Finney1998], WRF-Fire [...]), challenges remain in simulating large, long-duration fires efficiently and accurately capturing complex phenomena like fire-atmosphere feedback [...]. ForeFire addresses these needs by providing a high-performance, open-source C++ engine built with parallelism (MPI) and direct atmospheric coupling in mind. Its architecture allows for [... specific advantages ...], enabling research into [... specific research areas ...] and providing a flexible platform for [... specific applications ...]. Compared to [...], ForeFire offers [...]. The availability of an open-source C++ core facilitates community contributions and the integration of novel physical models. -->

# Acknowledgements

<!--
Instructions for this section:
- Acknowledge any individuals who contributed significantly but are not authors (e.g., testing, initial ideas).
- Acknowledge any funding sources (grants, agencies) that supported the development of ForeFire. Include grant numbers if applicable.
- Mention if sponsors had any role in the study design, data collection/analysis, decision to publish, or preparation of the manuscript. (JOSS requires disclosure).
-->

*TODO: Add acknowledgements for funding, significant non-author contributions, etc.*
<!-- 
We acknowledge contributions from [...] during the genesis of this project. This work was supported by [Funding Agency name, Grant Number XXX] and [Other funding sources]. The sponsors had no role in [... state involvement or lack thereof ...]. We thank the anonymous reviewers for their constructive feedback. -->

# References

<!--
- This heading is required.
- Pandoc (the tool JOSS uses) will automatically generate the reference list here based on the citations used in the text above (e.g., [@Filippi2014]) and the entries in paper.bib.
- You don't write the reference list manually here.
-->