def _are_item_timeslots_free(subject_class,
							 conflicting_classes,
							 starting_index,
							 period_length,
							 timetable,
							 room):
	timeslots = list(timetable.timeslots)
	for i in range(starting_index, starting_index + period_length):
		try:
			timeslot = timeslots[i][0]
		except IndexError:
			return False

		# Check if a conflicting class has been scheduled in the same
		# timeslot.
		timeslot_classes = timetable.get_classes_in_timeslot(timeslot)
		if (not conflicting_classes.isdisjoint(timeslot_classes)
				or timetable.has_class(timeslot, room)):
			# We have a conflicting class in the current timeslot.
			return False

	return True


def _shuffle_slice(l, start, stop):
	# Obtained from https://stackoverflow.com/a/31416673/1116098.
	i = start
	while i < stop - i:
		idx = random.randrange(i, stop)
		l[i], l[idx] = l[idx], l[i]

		i += 1