
/*************************************************************************
 *                                                                       *
 *                        This work is licensed under a                  *
 *   CC BY-SA        Creative Commons Attribution-ShareAlike             *
 *                           3.0 Unported License.                       *
 *                                                                       * 
 *  Author: Di Paola Martin Pablo, 2012                                  *
 *                                                                       *
 *************************************************************************/

/*******************************************************************************
 *                                                                             *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS        *
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT          *
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A    *
 *  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER  *
 *  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,   *
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,        *
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR         *
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF     *
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING       *
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         *
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               *
 *                                                                             *
 *******************************************************************************/


#include <sys/msg.h>
#include <syslog.h>
#include "oserror.h"

#include "messagequeue.h"
#include "commonipc.h"

void MessageQueue::destroy() {
   syslog(LOG_DEBUG, "[Debug] %s message queue using the path %s with key %x.", "Destroying", path.c_str(), key);
   if(msgctl(fd, IPC_RMID, 0) != 0) {
      throw OSError("The message queue "
            MESSAGE_Key_Path_Permissions
            " cannot be destroyed",
            key, path.c_str(), permissions);
   }
}

MessageQueue::MessageQueue(const char *absolute_path, 
      int permissions, bool create) : owner(create),
   path(absolute_path), 
   permissions(permissions) {
      key = get_key(absolute_path);
      syslog(LOG_DEBUG, "[Debug] %s message queue using the path %s with key %x.", (create? "Creating" : "Getting"), absolute_path, key);
      fd = msgget(key, (create ? (IPC_CREAT | IPC_EXCL) : 0) | permissions);
      if(fd == -1) {
         throw OSError("The message queue %s "
               MESSAGE_Key_Path_Permissions,
               (create ? "cannot be created" : "does not exist"), 
               key, path.c_str(), permissions);
      }
   }

void MessageQueue::push(const void *msg, size_t size_txt) {
   if(msgsnd(fd, msg, size_txt, 0) == -1) {
      throw OSError("The message (text) of %i bytes in the queue "
            MESSAGE_Key_Path_Permissions
            " cannot be sent", 
            size_txt, 
            key, path.c_str(), permissions);
   }
}

ssize_t MessageQueue::pull(void *msg, size_t max_size_txt, long type) {
   ssize_t copyied = msgrcv(fd, msg, max_size_txt, type, 0);
   if(copyied == -1) {
      throw OSError("The message (text) of at most %i bytes and type '%i' in the queue "
            MESSAGE_Key_Path_Permissions
            " cannot be recieved", 
            max_size_txt, type, 
            key, path.c_str(), permissions);
   }
   return copyied;
}


MessageQueue::~MessageQueue() 
   try {
      if(owner) {
         destroy();
      }
   } catch(const std::exception &e) {
      syslog(LOG_CRIT, "[Crit] An exception happend during the course of a destructor:\n%s", e.what());
   } catch(...) {
      syslog(LOG_CRIT, "[Crit] An unknow exception happend during the course of a destructor.");
   }

