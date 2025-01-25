from flask import request, jsonify, render_template
from app import app
import download_ms from myTools

@app.route('/process', methods=['POST'])
def process_task():
    # 获取 Electron 发送的 JSON 数据
    data = request.json
    print('data: ', data)
    task_name = data.get('taskName', '')
    params = data.get('taskInfo', {})

    if task_name:
        if params.taskType == 'download':
            pass
    else:
        result = "Unknown task"

    # 返回结果给 Electron
    return jsonify({'taskName':task_name, 'status': 'success'})

@app.route('/test', methods=['GET'])
def index():  # put application's code here
    user = {'username': 'Qciris'}
    return render_template('test.html', user=user)