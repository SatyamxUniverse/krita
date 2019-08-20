import sys
import os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "Dependencies")))
from .batch_exporter import registerDocker  # noqa

registerDocker()
