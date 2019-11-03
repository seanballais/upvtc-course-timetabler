import os


# DB_FILE is relative to the project directory.
APP_DATA_DIR = os.path.expanduser('~/.upvtc_ct/')
LOG_FILE = os.path.join(APP_DATA_DIR, 'app.log')
DB_FILE = os.path.join(APP_DATA_DIR, 'app.db')
