#!/usr/bin/env python3
"""Pong v0.1 TCP rendezvous/relay node; not a master/authority or DHT.
Both PS5s dial outward, so home routers require no forwarding.
A reachable relay is still required; run multiple independent peers later.
"""
import asyncio
import os
import re
import secrets
import time

HOST=os.environ.get("PONG_BIND","0.0.0.0")
PORT=int(os.environ.get("PONG_PORT","39733"))
MAX_LINE=512
NAME=re.compile(r"^[A-Za-z0-9 _-]{1,24}$")
rooms={}
class Client:
    def __init__(self,w):
        self.w=w;self.name="";self.room=None;self.last=time.monotonic()
    async def send(self,msg):
        if not self.w.is_closing():
            self.w.write((msg+"\n").encode())
            await self.w.drain()

async def handler(reader,writer):
    me=Client(writer)
    try:
        await me.send("HELLO|PONG1")
        while True:
            data=await asyncio.wait_for(reader.readline(),timeout=60)
            if not data or len(data)>MAX_LINE:break
            line=data.decode("ascii","ignore").strip()
            fields=line.split("|",3)
            op=fields[0]
            me.last=time.monotonic()
            if op=="NAME" and len(fields)==2 and NAME.fullmatch(fields[1]):
                me.name=fields[1];await me.send("OK|NAME")
            elif op=="LIST":
                await me.send("LIST_BEGIN")
                for code,room in list(rooms.items()):
                    if room["guest"] is None:
                        await me.send(f"ROOM|{code}|{room['title']}|{room['host'].name}")
                await me.send("LIST_END")
            elif op=="CREATE" and me.name and len(fields)==2 and NAME.fullmatch(fields[1]) and not me.room:
                code=secrets.token_hex(3).upper()
                rooms[code]={"title":fields[1],"host":me,"guest":None}
                me.room=code
                await me.send("CREATED|"+code)
            elif op=="JOIN" and me.name and len(fields)==2 and not me.room:
                code=fields[1].upper();room=rooms.get(code)
                if not room or room["guest"] or room["host"]==me:await me.send("ERROR|ROOM_UNAVAILABLE");continue
                room["guest"]=me;me.room=code
                await me.send("JOINED|"+code+"|GUEST")
                await room["host"].send("PEER_JOINED|"+me.name)
            elif op=="START" and me.room and rooms.get(me.room,{}).get("host")==me:
                peer=rooms[me.room]["guest"]
                if peer:
                    await me.send("STARTED")
                    await peer.send("STARTED")
            elif op in ("INPUT","STATE") and me.room and len(fields)>=2:
                room=rooms.get(me.room)
                if not room:continue
                if op=="INPUT" and me==room["guest"] and fields[1] in ("-1","0","1"):
                    await room["host"].send(line)
                if op=="STATE" and me==room["host"] and room["guest"] and len(line)<=240:
                    await room["guest"].send(line)
            elif op=="PING":await me.send("PONG")
            elif op=="LEAVE":
                break
            else:await me.send("ERROR|INVALID")
    except (asyncio.TimeoutError,ConnectionError,UnicodeError):
        pass
    finally:
        code=me.room
        if code in rooms:
            room=rooms[code]
            other=room["guest"] if me==room["host"] else room["host"]
            if other:
                other.room=None
                try:await other.send("PEER_LEFT")
                except (ConnectionError,OSError):pass
            del rooms[code]
        writer.close()
        try:await writer.wait_closed()
        except (ConnectionError,OSError):pass

async def main():
    server=await asyncio.start_server(handler,HOST,PORT,limit=MAX_LINE+1)
    print(f"Pong relay on {HOST}:{PORT}")
    async with server:await server.serve_forever()
if __name__=="__main__":asyncio.run(main())
