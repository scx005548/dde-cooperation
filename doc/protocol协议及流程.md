### 1.pair.proto
主要定义设备扫描协议、服务停止协议、设备协同配对协议。
```
1）设备扫描协议：ScanRequest->ScanResponse（UDP）
设备扫描协议，固定端口51595发送广播通知 ScanRequest，携带固定值key("UOS-COOPERATION"),设备信息，以及需要在配对连接tcp的端口号port。
服务启动，需要有tcp的服务器，并开启监听广播出去的端口号port，以供其他机器配对请求使用。

当一端收到另一端的ScanRequest消息后，需要回复ScanResponse,携带同样的消息，这样才可以双方机器都有对方设备信息。

//!!!维护设备是否在线，需要定时发送ScanRequst。uos端定时10s发送，超时25s离线，从本地设备列表清除。
//!!!为感知对方是否存在，会定时发送ScanRequest消息，当接受到该消息后，需要更新其中的设备信息和端口号，以防止对方在超时时间内重启或改变了设备信息。


2）设备协同配对协议：PairRequest->PairResponse（TCP）
设备协同机器配对请求协议：PairRequest，请求配对的一方发送，携带固定值key（"UOS-COOPERATION"），和设备信息。
设备协同请求配对前先使用tcp连接对方，然后发送PairRequest消息。

当一端收到另一端的PairRequest消息后，需要回复PairResponse消息，里面携带携带固定值key（"UOS-COOPERATION"），和设备信息，以及是否同意配对请求，
无论是同意还是不接受，都需要回复PairResponse。

3）跨端协同服务停止通知：ServiceStoppedNotification（UDP）
uos端有停止跨端协同服务的总开关，当打开本开关后，停掉一切服务，不响应任何消息。当一端接收到本消息，则应该断开所有连接，关掉设备共享功能，当设备离线处理。
```

```asm
遗留问题：
// 1. ScanRequest消息中未有协议版本信息：建议添加protoVersion。当扫描到设备与本地协议版本号不匹配，则不用添加该设备到本地设备列表。

TODO:建议修改
// 1.为了避免有安全漏洞，跨端服务不应该暴露tcp监听端口。
// 2.PairRequest只用发送设备id（uuid)，本机已经报存对端所有信息。
// 3.PairResponse建议修改如下：。
// 4.无需StopPairRequest和StopPairResponse。

// UDP广播，端口号为 51595
message ScanRequest
{
    string key = 1;             // 固定值 "UOS-COOPERATION"
    string protoVersion = 2;    // 协议版本
    DeviceInfo deviceInfo = 3;
}

// UDP，向 ScanRequest 来源的 IP:PORT 返回
message ScanResponse
{
    string key = 1;             // 固定值 "UOS-COOPERATION"
    string protoVersion = 2;    // 协议版本
    DeviceInfo deviceInfo = 3;
}

// UDP，连接 ScanResponse 返回的端口
message PairRequest
{
    string key = 1;             // 固定值 "UOS-COOPERATION"
    string deviceUuid = 2;
}

// 收到paireResponse的port后，主动去连接tcp socket
// UDP 配对时返回
message PairResponse
{
    string key = 1;             // 固定值 "UOS-COOPERATION"
    string deviceUuid = 2;
    bool agree = 3;             // 是否同意配对
    int32 port = 4;
}
```
协议：

| 协议   | 发送  | 回复                                                                                                                                                                          |
|------|-----|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| 设备扫描 | // UDP广播，端口号为 51595<br>message ScanRequest<br>{<br>string key = 1; // 固定值 "UOS-COOPERATION"<br>DeviceInfo deviceInfo = 2;<br>int32 port = 3; // TCP 端口,<br>}<br> | // UDP，向 ScanRequest 来源的 IP:PORT 返回<br>message ScanResponse<br>{<br>string key = 1; // 固定值 "UOS-COOPERATION"<br>DeviceInfo deviceInfo = 2;<br>int32 port = 3;// TCP 端口<br>} 
| 协同配对 |// TCP，连接 ScanResponse 返回的端口<br>message PairRequest<br>{<br>string key = 1; // 固定值 "UOS-COOPERATION"<br>DeviceInfo deviceInfo = 2;<br>}| // 配对时返回<br>message PairResponse<br>{<br>string key = 1; // 固定值 "UOS-COOPERATION"<br>DeviceInfo deviceInfo = 2;<br>bool agree = 3; // 是否同意配对<br>}                           
| 服务停止 |// UDP，服务停止通知<br>message ServiceStoppedNotification<br>{<br>string deviceUuid = 1;<br>}| 无                                                                                                                                                                           |

 基础类型定义:
```asm
// 定义设备类型
enum DeviceOS
{
DEVICE_OS_OTHER = 0;
DEVICE_OS_UOS = 1;
DEVICE_OS_LINUX = 2;
DEVICE_OS_WINDOWS = 3;
DEVICE_OS_MACOS = 4;
DEVICE_OS_ANDROID = 5;
}

// 显示服务类型，x11，wayland
enum Compositor
{
COMPOSITOR_NONE = 0;
COMPOSITOR_X11 = 1;
COMPOSITOR_WAYLAND = 2;
}

//设备信息
message DeviceInfo
{
string uuid = 1;
string name = 2;            // 扫描发起者的设备名
DeviceOS os = 3;            // 扫描发起者的操作系统
Compositor compositor = 4;  // 扫描发起者的显示服务器类型
}
```
--- 
暂无使用:
```asm
// 从带外数据发送
message StopPairRequest
{
bool immediately = 1;   // 是否立即断开,立即断开时不返回 response
}

message StopPairResponse
{
string bye = 1; // 固定值 "UOS-BYE"
}
```

### 2.service_status.proto
服务状态通知协议，通知对方本端共享剪切板和共享设备功能是否打开。

---
```
1)windows端需要知道uos端共享剪切板和共享设备功能状态。
uos<->uos:单边控制逻辑，A端打开则可共享本端设备，剪切板和输入事件可流转到B端，这个时候不管B端是否打开两者功能。如果A端打开，B端关闭，则A端可流转事件到B端，B端则不可流转事件到A端。
uos->windows:uos端控制该功能。uos打开则等同windows端打开，uos端关闭等同windows关闭该功能。

2)设备pair后，uos端会发送共享剪切板和共享设备开关的状态。当用户手动设备两者状态，也会同步发送开关状态到pair的机器上。对端无需回复消息。
```
```asm
// windows need know uos-end sharedClipboard and sharedDevice status
// TCP; paired 后发送；服务状态变化发送
message ServiceOnOffNotification
{
    bool sharedClipboardOn = 1;
    bool sharedDevicesOn = 2;
}
```
### 3.clipboard.proto
剪切板数据传输协议。分为剪切板数据通知协议、内容请求协议和内容回复协议。

---
```
1）剪切板数据通知协议，主要发送target数据：
// targets:
// STRING
//		剪切板文本内容字符串（ANSI），当剪切板内容是文件时，为文件路径列表，以换行分隔
// UTF8_STRING
//		同 STRING，编码为 UTF-8
// text/plain
//		剪切板文本内容（CRLF）
// text/plain;charset=utf-8
//		剪切板文本内容（CRLF UTF-8）
// text/html
//		剪切板 HTML 内容
// image/*
//		剪切板图片内容
// text/uri-list
//		URI 列表，多个 URI 以换行分隔
// x-special/gnome-copied-files
//		仅当复制文件时有，第一行为「copy」，其余同 text/uri-list

注意点：
a.uos上支持的文件target为：x-special/gnome-copied-files，内容第一行为copy或cut，第二行为文件路径。如：copy\nC:/uos.txt。
b.对于windows机器，发送文件target后，uos端需要填充text/uri-list，以及内容第二行文件路径（处理过后，有mountPoint）为填充内容。
c.发送图片数据，windows发送的target为image/bmp,数据内容需要填充文件头后（windows端处理），uos端才能在画板上粘贴。
```
协议：

| 协议            | 发送                                                             | 回复                                                                                                                                                                          |
|---------------|----------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| 剪切板数据target通知 | message ClipboardNotify {<br>repeated string targets = 1;<br>} | 无
| 剪切板数据请求       | message ClipboardGetContentRequest {<br>string target = 1;<br>} | message ClipboardGetContentResponse {<br>string target = 1;<br>bytes content = 2;<br>}                                                                              |

### 4.fs.proto 文件传输协议
主要定义文件传输相关的协议。

---
```
1)FsRequest：当双方pair连接后，就应该发送FsRequest，建立文件传输通道。单向传输。A->B传输文件为一条tcp通道，B-A传输文件为一条tcp通道。
2)接收到FsRequest后，需要将文件传输通道的tcp服务端口传输过去，发送FsResponse
message FsResponse {
    int64 serial = 1;   // 序号
    bool accepted = 2;
    uint32 port = 3;
}
3)接受到FsResponse后，需要建立tcp客户端使用port去连接对端的服务端，建立单向文件传输通道。
```
协议：

| 协议       | 发送                                                                                   | 回复                                                                                                                                                                                                                          |
|----------|--------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| 文件传输通道建立 | message FsRequest {<br>int64 serial = 1;// 序号<br>optional string path = 2;<br>}      | message FsResponse {<br>int64 serial = 1;   // 序号<br>bool accepted = 2;<br>uint32 port = 3;<br>}                                                                                                                            
| 发送文件请求   | message FsSendFileRequest {<br>int64 serial = 1;// 序号<br>string path = 2;// 文件名<br>} | message FsSendFileResponse {<br>int64 serial = 1;// 序号<br>bool accepted = 2;<br>}<br><br>//中间走文件内容请求协议操作<br><br>// 回复文件发送结果<br>message FsSendFileResult {<br>int64 serial = 1;<br>string path = 2;<br>bool result = 3;<br>} |

文件內容请求相关协议：

| 协议      | 发送                                                                                   | 回复                                                                                                                                                                                                                         |
|---------|--------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| getAttr | message FsMethodGetAttrRequest {<br>int64 serial = 1; // 序号<br>string path = 2;<br>optional FsFileInfo fi = 3;<br>}      | message FsMethodGetAttrResponse {<br>int64 serial = 1;   // 序号<br>int32 result = 2;<br>FsStat stat = 3;<br>}                                                                                                                          
| ReadDir | message FsMethodReadDirRequest {<br>int64 serial = 1;   // 序号<br>string path = 2;<br>uint64 offset = 3;<br>optional FsFileInfo fi = 4;<br>} | message FsMethodReadDirResponse {<br>int64 serial = 1;   // 序号<br>int32 result = 2;<br>repeated string item = 3;<br>} |
| Read    | message FsMethodReadRequest {<br>int64 serial = 1;   // 序号<br>uint64 offset = 2;<br>uint64 size = 3;    // max 2GB<br>optional FsFileInfo fi = 4;<br>} | message FsMethodReadResponse {<br>int64 serial = 1;   // 序号<br>int32 result = 2;<br>bytes data = 3;<br>} |
| Open    | message FsMethodOpenRequest {<br>int64 serial = 1;   // 序号<br>string path = 2;<br>optional FsFileInfo fi = 3;<br>} | message FsMethodOpenResponse {<br>int64 serial = 1;   // 序号<br>int32 result = 2;<br>optional uint64 fh = 3;<br>} |
| Release | message FsMethodReleaseRequest {<br>int64 serial = 1;   // 序号<br>string path = 2;<br>optional FsFileInfo fi = 3;<br>} | message FsMethodReleaseResponse {<br>int64 serial = 1;   // 序号<br>int32 result = 2;<br>} |

基础数据类型定义：
```asm
// 参考/usr/include/x86_64-linux-gnu/bits/struct_stat.h里面定义的 struct stat
message FsStat {
    uint64 ino = 1;
    uint64 nlink = 2;
    uint32 mode = 3;    // file type and mode
    uint32 uid = 4;
    uint32 gid = 5;
    uint64 size = 6;    // total size, in bytes
    uint64 blksize = 7;
    uint64 blocks = 8;
    google.protobuf.Timestamp atime = 9;    // last access of file data
    google.protobuf.Timestamp mtime = 10;    // last modification of file data
    google.protobuf.Timestamp ctime = 11;    // last status change timestamp (time of last change to the inode)
}

message FsFileInfo {
    int32 flags = 1;
    bool writepage = 2;
    bool direct_io = 3;
    bool keep_cache = 4;
    bool flush = 5;
    bool nonseekable = 6;
    bool cache_readdir = 7;
    uint64 fh = 8;
    uint64 lock_owner = 9;
    uint32 poll_events = 10;
    bool noflush = 11;
}
```
### 5.device_sharing.proto 设备共享协议
主要定义设备共享请求、设备位置设置、鼠标流转、事件传输协议。

---
```
设备共享概念：
1).一端打开设备共享功能，则本机输入输出事件可流转到已协同上的对端机器，协同上的对端机器响应发送过来的输入输出事件，否则不响应对端发送过来的事件协议数据。
2).a端打开，B端未开启，如果鼠标类事件流转到B端，需要判断从对端鼠标类事件流转回来的触发条件，是A端鼠标操作还是本机操作。如果是A端，则允许流转回，B端只能操作本机。
3).双方都打开设备共享功能，设备共享连接后，默认未开启真正的设备共享，当一端鼠标流转出去后，才实质发送输入事件。本端操作鼠标流转回来后，则各自设备只操作本机（相当于未开启设备共享功能）。

发送消息注意：
1)设备共享前，需要发送设备共享连接请求；
2)设备共享请求后，uos端会发送默认相对位置消息，如果用户手动设置后，会主动发送到对方用户设置的位置消息（FlowDirectionNtf)。默认发起方在左边（从屏幕右边流转出），接收方在右边（从屏幕左边流转出）。
如：当发起方通知对端FLOW_DIRECTION_RIGHT，则表明对端在右边，但对端机器的鼠标流转方向为屏幕的左边。

和windows探讨点：
1. 外设共享总开关和 DeviceSharingStartRequest 的关系：
   在设备配对成功后可以发送 DeviceSharingStartRequest 消息，无论总开关是否开启都要发送。
   总开关的作用只控制是否转发本机的外设消息到对端，设备只要配对和设备共享请求成功了，就要接收对端发送的消息并正常响应，比如响应鼠标移动消息，鼠标按键和键盘按键消息。

2. 鼠标流转规则
   鼠标出屏的流转规则可以简化为如下逻辑：
   1. 对于接收到 FlowRequest 消息的一端，也就是鼠标即将要流入的一端，不转发外设消息，响应本机和接收到的远端外设消息
   2. 对于发送 FlowRequest 消息的一端，也就是鼠标流出的一端，分两种情况：
   a. 本机鼠标引起的， 转发外设消息
   b. 对端发送来的鼠标消息引起的，不转发外设消息。
```
协议：

| 协议      | 发送             | 回复     |
|---------|----------------|--------
| 设备共享请求  | message DeviceSharingStartRequest {<br>} | message DeviceSharingStartResponse {<br>bool accept = 1;<br>} 
| 设备共享停止请求 | message DeviceSharingStopRequest {<br>} | message DeviceSharingStopResponse {<br>} |
| 设备位置设置  | message FlowDirectionNtf {<br>FlowDirection direction = 1;<br>} | 无      |
| 流转请求    | message FlowRequest {<br>int64 serial = 1; // 序号<br>FlowDirection direction = 2;<br>uint32 x = 3;<br>uint32 y = 4;<br>} | // 暂未使用<br>message FlowResponse {<br>int64 serial = 1; // 序号<br>bool success = 2; // 是否成功<br>} |
| 输入事件    | // 事件类型、事件码、事件值和 Linux input event 一致，<br>// Linux 上直接写入设备即可，其他平台需要解析转换<br>message InputEventRequest {<br>int64 serial = 1;// 序号<br>DeviceType deviceType = 2;<br>int32 type = 3;// 事件类型<br>int32 code = 4;// 事件码<br>int32 value = 5;    // 事件值<br>} |    // 暂未使用<br>message InputEventResponse {<br>int64 serial = 1; // 序号<br>bool success = 2; // 是否成功<br>} 

基础数据定义：
```asm
// 设备类型
enum DeviceType {
    DEVICE_TYPE_UNKNOWN = 0;
    DEVICE_TYPE_KEYBOARD = 1;
    DEVICE_TYPE_MOUSE = 2;
    DEVICE_TYPE_TOUCHPAD = 3;
}

// 方向位置定义
enum FlowDirection {
    FLOW_DIRECTION_TOP = 0;
    FLOW_DIRECTION_RIGHT = 1;
    FLOW_DIRECTION_BOTTOM = 2;
    FLOW_DIRECTION_LEFT = 3;
}

```