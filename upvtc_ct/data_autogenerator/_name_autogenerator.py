#!/usr/bin/env python
import logging
import os
import sys
import threading

import requests

from upvtc_ct import settings


app_logger = logging.getLogger()


def autogenerate_names():
    names = _get_names(100, 16)
    with open(settings.NAMES_FILE, 'w') as f:
        for name in names:
            f.write(f'{name}\n')


def _get_random_name(names, num_names):
    for _ in range(num_names):
        while True:
            try:
                r = requests.get('https://api.namefake.com')
                name = r.json()['name']
            except requests.exceptions.ConnectionError:
                app_logger.warning(
                    'Connection failed while obtaining random name. '
                    'Retrying...')
                continue

            names.add(name)
            break


def _get_names(num_names, num_threads):
    names = set()

    # Split the task of generating names among threads equally.
    num_names_per_thread = num_names // num_threads
    num_remaining_names = num_names % num_threads

    threads = list()
    for i in range(num_threads):
        num_names = num_names_per_thread
        if i == (num_threads - 1) and num_remaining_names > 0:
            # There are more names that needs creating than we can split among
            # threads equally. Let the last thread create those remaining
            # names.
            num_names += num_remaining_names

        t = threading.Thread(target=_get_random_name, args=(names, num_names,))
        threads.append(t)
        t.start()

    for thread in threads:
        thread.join()

    return names
