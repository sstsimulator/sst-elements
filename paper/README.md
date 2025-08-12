# Joss Paper

This paper was written based on SST 15.0. It gives a basic overview of the SST project's structure and goals.

## Building

The paper is built using the `inara` container.

The container can be downloaded from Docker Hub [here](https://hub.docker.com/r/openjournals/inara).
To launch the container, follow instructions on the GitHub repo [here](https://github.com/openjournals/inara).

When inside the container, build the pdf with the following command:

```
inara path/to/paper.md
```

A draft pdf is created by default. Use the `-p` flag to create a production pdf.
