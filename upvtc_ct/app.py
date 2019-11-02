import os

import docopt

from upvtc_ct import __version__, models, settings


def main():
	# Set up argument parsing and obtain arguments through docopt.
	doc_string = (
		f'Automated Course Timetabler for UPVTC (v{ __version__ })'
		 ''
		 'Usage:'
		 '  upvtc_ct'
		 '  upvtc_ct (-h | --help)'
		 '  upvtc_ct --version'
		 ''
		 'Options:'
		 '  -h --help   Show this help text.'
		 '  --version   Show version.')
	arguments = docopt.docopt(doc_string, version=__version__)

	# Make sure we have an application folder already. We store our database
	# file in the application folder.
	if not os.path.isdir(settings.APP_DATA_DIR):
		# TODO: Use a log function instead of print().
		print(
			 '[INIT] Application folder non-existent. Creating at '
			f'{ settings.APP_DATA_DIR }... ', end='')
		
		os.mkdir(settings.APP_DATA_DIR)

		print('Done!')

	# Make sure we already have the database file.
	if not os.path.isfile(settings.DB_FILE):
		# TODO: Use a log function instead of print().
		print(
			 '[INIT] Application database file non-existent. Create database '
			f'file, { settings.DB_FILE }, at { settings.APP_DATA_DIR }... ',
			end='')

		models.setup_models()

		print('Done!')


if __name__ == '__main__':
	main()
