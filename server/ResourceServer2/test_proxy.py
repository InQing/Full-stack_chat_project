import requests
import base64
import json
import os
import hashlib

class ProxyTest:
    def __init__(self, proxy_url="http://localhost:9999"):  # nginx默认监听80端口
        self.base_url = proxy_url
        self.token = "57144af5-72fc-4b78-bd97-57268a5d86cc"  # 实际的token
        self.uid = "3"  # 用户ID
        
    def test_upload_init(self, file_id, filename):
        """测试上传初始化接口"""
        url = f"{self.base_url}/upload/init"
        headers = {
            "Authorization": self.token,
            "Content-Type": "application/json"
        }
        data = {
            "file_id": file_id,
            "filename": filename,
            "uid": self.uid
        }
        
        response = requests.post(url, json=data, headers=headers)
        print(f"\n=== 测试上传初始化 ===")
        print(f"请求URL: {url}")
        print(f"请求头: {headers}")
        print(f"请求数据: {data}")
        print(f"状态码: {response.status_code}")
        print(f"响应内容: {response.text}")
        return response.json() if response.status_code == 200 else None

    def test_chunk_upload(self, file_id, upload_id, chunk_data, chunk_number, total_chunks):
        """测试分片上传接口"""
        url = f"{self.base_url}/upload/chunk"
        headers = {
            "Content-Type": "application/json",
            "Upload-ID": upload_id  # 添加Upload-ID到请求头
        }
        
        # 计算分片MD5
        md5 = hashlib.md5(chunk_data).hexdigest()
        
        # Base64编码分片数据
        chunk_base64 = base64.b64encode(chunk_data).decode('utf-8')
        
        data = {
            "file_id": file_id,
            "upload_id": upload_id,
            "chunk": chunk_base64,
            "chunk_number": chunk_number,
            "total_chunks": total_chunks,
            "md5": md5,
            "uid": self.uid  # 确保uid也包含在请求体中
        }
        
        response = requests.post(url, json=data, headers=headers)
        print(f"\n=== 测试分片上传 {chunk_number}/{total_chunks} ===")
        print(f"请求URL: {url}")
        print(f"请求头: {headers}")
        print(f"状态码: {response.status_code}")
        print(f"响应内容: {response.text}")
        return response.status_code == 200

    def test_download(self, file_id):
        """测试下载接口"""
        url = f"{self.base_url}/download/{file_id}"
        headers = {
            "Authorization": self.token,
            "Content-Type": "application/json"
        }
        data = {
            "uid": self.uid
        }
        
        response = requests.get(url, json=data, headers=headers)
        print(f"\n=== 测试文件下载 ===")
        print(f"请求URL: {url}")
        print(f"请求头: {headers}")
        print(f"请求数据: {data}")
        print(f"状态码: {response.status_code}")
        print(f"Content-Length: {response.headers.get('Content-Length')}")
        print(f"Content-Type: {response.headers.get('Content-Type')}")
        return response.status_code == 200

def main():
    # 创建测试实例
    tester = ProxyTest()
    
    # 测试参数
    file_id = "test_file_001"
    filename = "test.txt"
    
    print("开始测试 - 使用以下凭证：")
    print(f"用户ID: {tester.uid}")
    print(f"Token: {tester.token}")
    
    # 1. 测试上传初始化
    init_result = tester.test_upload_init(file_id, filename)
    if not init_result:
        print("上传初始化失败")
        return
    
    upload_id = init_result["data"]["upload_id"]
    
    # 2. 测试分片上传
    # 创建测试数据
    test_data = b"Hello, this is a test file content."
    chunk_size = 10
    chunks = [test_data[i:i+chunk_size] for i in range(0, len(test_data), chunk_size)]
    total_chunks = len(chunks)
    
    for i, chunk in enumerate(chunks, 1):
        if not tester.test_chunk_upload(file_id, upload_id, chunk, i, total_chunks):
            print(f"分片 {i} 上传失败")
            return
    
    # 3. 测试下载
    if tester.test_download(file_id):
        print("\n所有测试完成")
    else:
        print("\n下载测试失败")

if __name__ == "__main__":
    main()