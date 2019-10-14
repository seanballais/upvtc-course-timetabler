FROM python:3.7.4-alpine

WORKDIR /usr/src/upvtc-course-timetabler

ENV PYTHONDONTWRITEBYTECODE 1
ENV PYTHONUNBUFFERED 1

# Install system dependencies
RUN apk update \
    && apk add --virtual build-deps gcc python3-dev musl-dev \
    && apk add postgresql-dev \
    && pip install psycopg2 \
    && apk del build-deps

# Install required Python packages
RUN pip install --upgrade pip
RUN pip install pipenv
COPY ./Pipfile /usr/src/upvtc-course-timetabler/Pipfile
RUN pipenv install --skip-lock --system --dev

# Copy project to the working directory.
COPY . /usr/src/upvtc-course-timetabler
