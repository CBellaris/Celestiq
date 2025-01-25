from app import app
import sys
import os

sys.path.append(os.path.abspath(os.path.dirname(__file__)))

if __name__ == '__main__':
    app.run(debug=True)