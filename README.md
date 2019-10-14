# upvtc-course-timetabler
A course timetabler for UPVTC and my undergraduate thesis.

## Notes
### Why is the configuration folder for the entire project (`.config/`) called `.config/`?
Some might wonder, "Why not just name it as `config/`?". However, doing so will make the folder seem like a Django app, which it is not. Prepending it with an underscore instead of a period, i.e. `_config/`, will make it seem like an internal Python package. The folder is obviously not a Python package nor it should. Using `.config/` is a good fit since it is only a folder containing environment variables for the project's Docker container, at least for now, and the folder's naming scheme follows the naming scheme of Docker Compose's environment file (`.env`).
