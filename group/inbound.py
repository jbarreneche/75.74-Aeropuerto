
import sys
import syslog

sys.path.append("../ipc")

from ipc import MessageQueue
from subprocess import Popen

import passage
import ring
import struct
import message
from invalid import *
import traceback
import time

import stop
import os

ALREADY_ADDR_USED_ATTEMPTS = 10
ALREADY_ADDR_USED_SLEEP = 15
addr_attempts = 0

import signal
def handler(signum, frame):
   raise KeyboardInterrupt("Interrupted by a TERM signal.")

signal.signal(signal.SIGTERM, handler)

def create_leader_proposal_msj(localhost_name):
   s = struct.pack(">HBB%is" % (len(localhost_name)), passage.TTL, passage.LOOP_SUBTYPE_BY_NAME['Leader'], len(localhost_name), localhost_name)
   return message.pack(s, passage.ID_BY_TYPE['LOOP'])


class Driver:
    def __init__(self, localhost_name, path, char_id_out, group_id, network_name, userland_inbound_queue):
        self.leader_name = self.localhost_name = localhost_name
        self.network_name = network_name
        self.leader_process = None
        self.path, self.char_id_out, self.group_id = path, char_id_out, group_id
        self.userland_inbound_queue = userland_inbound_queue

    def handle_loop_message(self, loop_payload, ttl):
        '''This function will process the loop message and will determine if the message should be passed to the next stage (the next process)
           or it should be discarted. (Returning True or False).
           '''
        type = struct.unpack(">B", loop_payload[0])[0]

        if type == passage.LOOP_SUBTYPE_BY_NAME['Leader']:
            syslog.syslog(syslog.LOG_DEBUG, "Leader Election received...")
            leader_name_len = struct.unpack('>B', loop_payload[1])[0]
            leader_name = struct.unpack('%is' % leader_name_len, loop_payload[2: leader_name_len+2])[0]

            if leader_name == self.localhost_name:
                syslog.syslog(syslog.LOG_DEBUG, "I'am the new leader %s (previous %s) of the group with %i members." % (str(self.localhost_name), str(self.leader_name), passage.TTL - ttl + 1))
                #Stop the leader algorithm. 
                #
                # The inbound process MUST start sending its localname as leadername to the outbound queue.
                # When that message come back to the outbound, then the algorithm finish and localname is the leadername
                self.leader_name = leader_name
                
                if self.leader_process and self.leader_process.poll() is None:
                   return False #iam the leader already

                self.clean()
                self.leader_process = Popen(["python", "leader.py", self.path, self.char_id_out, str(self.group_id), self.localhost_name, self.network_name])
               
                #XXX Who has the token?
                self.userland_inbound_queue.push(message.pack("\x00"*(4*3 + 256), passage.ID_BY_TYPE['USER']))
                return False 


            elif leader_name < self.localhost_name:
                syslog.syslog(syslog.LOG_DEBUG, "Minor leader proposal %s discarted by %s (actual leader %s)." % (str(leader_name), str(self.localhost_name), str(self.leader_name) ))
                return False
            else:
                syslog.syslog(syslog.LOG_DEBUG, "Mayor leader proposal %s exceeds me %s (previous leader %s)." % (str(leader_name), str(self.localhost_name), str(self.leader_name) ))
                self.clean()
                self.leader_name = leader_name
                return True

        elif type == passage.LOOP_SUBTYPE_BY_NAME['LinkBroken']:
            open_node_name_len = struct.unpack('>B', loop_payload[1])[0]
            open_node_name = struct.unpack('%is' % open_node_name_len, loop_payload[2: open_node_name_len+2])[0]

            if open_node_name == self.localhost_name:
               return False
            else:
               self.clean()
               return True
        else:
            #Tipo incorrecto, como llego aqui?!?
            raise Exception

        return False


    def clean(self):
       stop.stop(self.leader_process)
       self.leader_process = None


    def create_leader_proposal_msj(self):
       return create_leader_proposal_msj(self.localhost_name)

    def create_linkbroken_msj(self):
       s = struct.pack(">HBB%is" % (len(localhost_name)), passage.TTL, passage.LOOP_SUBTYPE_BY_NAME['LinkBroken'], len(localhost_name), localhost_name)
       return message.pack(s, passage.ID_BY_TYPE['LOOP'])

if __name__ == '__main__':
   if len(sys.argv[1:]) != 5:
      print "Usage: inbound.py path char_id_in group_id localhost_name network_name"
      print "  - path: a full path to a file to be used as part of the key for the in/out queues."
      print "  - char_id_in: an integer or a character (converted in an int later) to be used as a part of the key of the inbound queue. The id used by the outbound queue will be that id+128."
      print "  - group_id: the id of the group (a no-negative)"
      print "  - localhost_name: the name of this host viewed by other nodes."
      print "  - network_name: the name of the network, which message addressed to network_name will be delivery to any node in that network (broadcast address)."
      sys.exit(1)


   path, char_id_in, group_id, localhost_name, network_name = sys.argv[1:]
   group_id = int(group_id)
   assert group_id >= 0
   assert ":" not in localhost_name

   # The 'char' id can be an integer or a letter.
   try:
      char_id_in = int(char_id_in)
   except ValueError:
      char_id_in = int(ord(char_id_in))

   # From the value of one char, we calculate the other
   assert 0 <= char_id_in < 128
   char_id_out = char_id_in + 128

   # Because the MessageQueue constructor expect a 'char', we do the translate
   char_id_in = chr(char_id_in)
   char_id_out = chr(char_id_out)

   pid = str(os.getpid())

   syslog.openlog("inbound[%s]" % pid)
   syslog.syslog(syslog.LOG_INFO, "Init 'inbound' process. Creating queues. Arguments: Path: %s Char_in_id: %s GroupId: %i Localhost: %s NetworkName: %s PID: %s" % (
      path, hex(ord(char_id_in)), group_id, localhost_name, network_name, pid))
   userland_inbound_queue = MessageQueue(path, char_id_in, 0644, True)
   userland_outbound_queue = MessageQueue(path, char_id_out, 0644, True)

   localhost_name = ":".join([localhost_name, pid])
   
   driver = Driver(localhost_name, path, char_id_out, group_id, network_name, userland_inbound_queue)
   
   head_process = Popen(["python", "outbound.py", path, char_id_out, str(group_id), localhost_name])
   
   try:
      while True:
         try:
            syslog.syslog(syslog.LOG_INFO, "Pushing 'BrokenLink' in the output queue.")
            userland_outbound_queue.push(driver.create_linkbroken_msj())
            driver.clean()

            syslog.syslog(syslog.LOG_INFO, "Construction the ring")
            previous_node = ring.tail(network_name, group_id, localhost_name, driver)
            syslog.syslog(syslog.LOG_INFO, "External node %s connected to me" % str(previous_node.getpeername()))

            addr_attempts = 0
            syslog.syslog(syslog.LOG_INFO, "Pushing 'LeaderElection' in the output queue.")
            userland_outbound_queue.push(driver.create_leader_proposal_msj())

            syslog.syslog(syslog.LOG_INFO, "Connection ready. Forwarding the messages.")
            passage.passage_inbound_messages(previous_node, userland_inbound_queue, userland_outbound_queue, driver)

         except InvalidMessage, e:
            syslog.syslog(syslog.LOG_CRIT, "%s\n%s" % (traceback.format_exc(), str(e)))
         except UnstableChannel, e:
            syslog.syslog(syslog.LOG_CRIT, "%s\n%s" % (traceback.format_exc(), str(e)))
         except KeyboardInterrupt:
            syslog.syslog(syslog.LOG_INFO, "Interruption:\n%s" % traceback.format_exc())
            sys.exit(0)
         except Exception, e:
            syslog.syslog(syslog.LOG_CRIT, "Critical exception (will shutdown everything) %s\n%s" % (traceback.format_exc(), str(e)))

            if hasattr(e, 'errno') and e.errno == 98:
               addr_attempts += 1
               syslog.syslog(syslog.LOG_INFO, "Address already used (attempts %i)" % addr_attempts)
               if addr_attempts < ALREADY_ADDR_USED_ATTEMPTS:
                  time.sleep(ALREADY_ADDR_USED_SLEEP)
                  continue
            
            sys.exit(2)

   finally:
      syslog.syslog(syslog.LOG_INFO, "Shutdown 'inbound'. Stoping other processes.")
      time.sleep(0.1)
      stop.stop(head_process)
      driver.clean()

