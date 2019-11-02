import os


# DB_FILE is relative to the project directory.
APP_DATA_DIR = os.path.expanduser('~/.upvtc_ct/')
DB_FILE = os.path.join(APP_DATA_DIR, 'app.db')
