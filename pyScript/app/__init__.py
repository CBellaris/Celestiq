from flask import Flask
from pyScript.config import Config
from flask_cors import CORS
app = Flask(__name__)
app.config.from_object(Config)
# CSRF防护
CORS(app, resources={r"/*": {"origins": "*"}})

from app import routes