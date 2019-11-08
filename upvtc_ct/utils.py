import datetime
import types


def copy_func(f):
	""" Deep copy function f."""
	return types.FunctionType(
		f.__code__, f.__globals__, f.__name__, f.__defaults__, f.__closure__)


def current_datetime():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
