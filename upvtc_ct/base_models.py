from django.db import models


class Base(models.Model):
    """
    Base model for all models in this project.

    Based on the base model from Botos, an open source election system. See
    LICENSE for more information.
    """
    date_created = models.DateTimeField(
        'date_created',
        auto_now=False,
        auto_now_add=True,
        null=False
    )
    date_updated = models.DateTimeField(
        'date_updated',
        auto_now=True,
        auto_now_add=False,
        null=False
    )

    class Meta:
        abstract = True
