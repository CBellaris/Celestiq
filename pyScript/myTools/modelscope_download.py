from modelscope.hub.file_download import model_file_download

def download_ms(id, file, cache_dir):
    model_dir = model_file_download(model_id=id, file_path=file, cache_dir=cache_dir)