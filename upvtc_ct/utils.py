import datetime


def current_datetime():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
