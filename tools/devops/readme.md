## Alternative Dockerfiles

WIP

- Dockerfile without python to make the image smaller 
- Multistage build to make the also smaller (490 vs 620 mb)

```
docker build -f tools/docker/Dockerfile.multistage -t forefire:multistage .
```