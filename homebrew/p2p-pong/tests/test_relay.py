import asyncio
import unittest
from relay.relay import handler

class RelayTest(unittest.IsolatedAsyncioTestCase):
 async def test_room(self):
  server=await asyncio.start_server(handler,"127.0.0.1",0)
  port=server.sockets[0].getsockname()[1]
  async def connect():
   r,w=await asyncio.open_connection("127.0.0.1",port)
   self.assertEqual((await r.readline()).decode().strip(),"HELLO|PONG1")
   return r,w
  a,aw=await connect();b,bw=await connect()
  aw.write(b"NAME|Alice\nCREATE|TestRoom\n");await aw.drain()
  self.assertEqual((await a.readline()).strip(),b"OK|NAME")
  code=(await a.readline()).decode().strip().split("|")[1]
  bw.write(("NAME|Bob\nJOIN|"+code+"\n").encode());await bw.drain()
  self.assertEqual((await b.readline()).strip(),b"OK|NAME")
  self.assertTrue((await b.readline()).startswith(b"JOINED|"))
  self.assertTrue((await a.readline()).startswith(b"PEER_JOINED|"))
  aw.write(b"START\n");await aw.drain()
  self.assertEqual((await a.readline()).strip(),b"STARTED")
  self.assertEqual((await b.readline()).strip(),b"STARTED")
  bw.write(b"INPUT|1\n");await bw.drain()
  self.assertEqual((await a.readline()).strip(),b"INPUT|1")
  aw.close();bw.close();await aw.wait_closed();await bw.wait_closed()
  server.close();await server.wait_closed()

if __name__=="__main__":unittest.main()
