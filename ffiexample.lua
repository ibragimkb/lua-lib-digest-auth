local cjson = require("cjson")
local ffi = require("ffi")


ffi.cdef[[
      typedef struct { int code; const char *body; } rpc_result;
      int getErrorBufferSize();
      char* allocBuffer(size_t s);
      void freeBuffer(const char * p);
      rpc_result getRPCData(const char * url, const char *userpwd, const char *postdata, char * buffer, char *err_buffer, const int err_buffer_sz);
      const char* version();
]]

local digestproxy = ffi.load("./libdigestauth.so")


local url = "http://127.0.0.1:8081/json_rpc"
local userpwd = "USER:PASSWORD"

local err_buf, res_buf, data, retstr
local ok, postreq, err
local jsonrpc = {}
local paymentId = {"0000000000000000", "0000000000000000"}

-- build jsonrpc request
jsonrpc["jsonrpc"] = "2.0";
jsonrpc["id"] = "0"
jsonrpc["method"] = "get_bulk_payments"
jsonrpc["params"] = {}
jsonrpc["params"]["payment_ids"] = paymentId
jsonrpc["params"]["min_block_height"] = 0
ok, postreq, err = pcall(cjson.encode, jsonrpc)
if not ok then
		print("xmr: error encode json request")
		do return end
end

-- alloc mem for error buffer
err_buf = ffi.new("char[?]", digestproxy.getErrorBufferSize())
-- set GC for responce buffer
res_buf = ffi.gc(digestproxy.allocBuffer(1), digestproxy.freeBuffer)
-- send request&get data from rpc
data = digestproxy.getRPCData(url, userpwd, postreq, res_buf, err_buf, ffi.sizeof(err_buf))
-- unset finalizer
ffi.gc(res_buf, nil)
-- if data==nil free buffer, exit
if not data then
		digestproxy.freeBuffer(res_buf)
		print("Internal error")
		do return end
end

res_buf = nil
-- convert result to string
if (1 == data.code) then
		retstr = ffi.string(data.body)
else
		retstr = ffi.string(err_buf)
end
-- free alloc mem
digestproxy.freeBuffer(data.body)
data.body= nil

print("result:\n", retstr)
