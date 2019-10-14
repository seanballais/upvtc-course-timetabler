# Script to be used for productions. Removes the necessity of specifying the
# Docker Compose files.
#!/bin/sh

docker-compose -f docker-compose.yml -f docker-compose.prod.yml $@
