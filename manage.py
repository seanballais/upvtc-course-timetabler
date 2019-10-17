#!/usr/bin/env python
"""Django's command-line utility for administrative tasks."""
import os
import sys


def read_env_file(env_file: str):
    # Based on `manage.py` in https://gist.github.com/bennylope/2999704, but
    # with the parser being implemented differently.
    try:
        with open(env_file) as f:
            content = f.read()
    except IOError as e:
        print(f"❌ Error reading environment file '{env_file}': {str(e)}")
        sys.exit(-1)

    line_number = 0
    for line in content.splitlines():
        line_number += 1

        if line.startswith('#'):
            # Line is a comment.
            continue

        try:
            key, value = line.lstrip().split('=', 1)
        except ValueError:
            print(f"❌ Key-value pair at line {line_number} is invalid.")
            sys.exit(-1)

        # Remove string definition quotations, if any, from the variable value
        # since they're not supposed to be in the value.
        #
        # TODO: Add string definition quotation validations just like in
        #       shells, such as Bash and ZSH.
        if (value.startswith('"') and value.endswith('"') or
                value.startswith('\'') and value.endswith('\'')):
            value = value[1:-1]

        os.environ.setdefault(key, value)


def main():
    os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'upvtc_ct.settings')
    try:
        from django.core.management import execute_from_command_line
    except ImportError as exc:
        raise ImportError(
            "Couldn't import Django. Are you sure it's installed and "
            "available on your PYTHONPATH environment variable? Did you "
            "forget to activate a virtual environment?"
        ) from exc

    read_env_file('.config/web.env')
    read_env_file('.config/db.env')
    execute_from_command_line(sys.argv)


if __name__ == '__main__':
    main()
