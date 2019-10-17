FROM python:3.7.4-alpine

WORKDIR /usr/src/upvtc-course-timetabler

ENV PYTHONDONTWRITEBYTECODE 1
ENV PYTHONUNBUFFERED 1

# Install system dependencies.
RUN apk update
RUN apk add --virtual build-deps gcc python3-dev musl-dev
RUN apk add postgresql-dev

# Install required Python packages.
RUN pip install --upgrade pip
RUN pip install pipenv
COPY ./Pipfile /usr/src/upvtc-course-timetabler/Pipfile
RUN pipenv install --skip-lock --system

# Remove build dependencies.
RUN apk del build-deps

# Copy project to the working directory.
COPY . /usr/src/upvtc-course-timetabler
