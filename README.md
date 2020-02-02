# C2C_Server

Author: Jeff Wilson
    Release Date: 4 December 2019

    Server side of the C2C application that will handle and maintain a list of active agents.
    List of commands that the agent can use via their own command line args:
        #JOIN: Agent is requesting to join the server.
            Server will reply with "$OK" or "$ALREADY MEMBER" depending on the agents previous status with server.
        #LEAVE: Agent is requesting to leave the server.
            Server will reply with "$OK" or "$NOT MEMBER" depending on the agents previous status with server.
        #LIST: Agent is requesting the server to list the server status (Agent must be active on server).
            Server will display a list of all active agents on the server.
            FORMATTING:
                <agent_IP, seconds_online>
                <next_agent_IP, seconds online>
        #LOG: Agent is requesting to be sent a log file from the server.
            The server will be maintaining a log file (log.txt) and can send it at an Agents request.
            FORMATTING (using example IP Addresses):
                "TIME": Received a "#JOIN" action from agent "147.26.143.22"
                "TIME": Responded to agent "147.26.143.22" with "$OK"
                "TIME": Received a "#LIST" from agent "147.26.143.22"
                "TIME": No response is supplied to agent "147.26.12.11" (agent not active)
