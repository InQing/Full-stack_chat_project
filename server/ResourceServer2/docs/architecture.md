客户端需求：使用 Qt 编写可视化界面，支持上传单个或多文件，下载单个或多个文件的功能

基本流程 1.上传文件：选择文件后，点击发送按钮，为每个文件的发送创建 UplodaFile 任务。UploadFile 中，将文件分片，每个分片都放入线程池中并行传输至服务器（流式传输），QNetworkAccessManager 默认支持 HTTP Keep-Alive，因此 http 连接可复用，开销小。服务器会对每个请求进行回包，若成功，则客户端更新文件上传进度。 2.下载文件：选择文件后，点击下载按钮，为每个文件的下载创建 DownLoadFile 任务，并放入线程池中执行，文件的下载为流式传输，需显示下载进度。

json 数据格式：
**上传文件请求：**

```json
{
  "file_id": "unique_file_id_12345",
  "chunk": "base64_encoded_chunk_data",
  "chunk_number": 1,
  "total_chunks": 10,
  "md5": "d41d8cd98f00b204e9800998ecf8427e"
}
```

- `file_id`: 唯一标识文件的 ID，服务器通过该 ID 识别文件。
- `chunk`: 当前上传的文件分片数据，采用 Base64 编码。
- `chunk_number`: 当前分片的编号，从 1 开始。
- `total_chunks`: 文件总的分片数。
- `md5`: 当前分片的 MD5 校验值，用于确保分片的完整性。

**下载文件请求：**

```json
{
  "file_id": "unique_file_id_12345"
}
```

- `file_id`: 要下载的文件 ID，服务器用该 ID 查找文件并开始下载。
