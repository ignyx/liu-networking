#!/usr/bin/env python
import GuiTextArea, RouterPacket, F, RouterSimulator
from copy import deepcopy

class RouterNode():
    myID = None
    myGUI = None
    sim = None
    costs = None
    routeTable = None #The path : which routers we went through to transmit the packet
    distanceTable = None 

    # Access simulator variables with:
    # self.sim.POISONREVERSE, self.sim.NUM_NODES, etc.

    # --------------------------------------------------
    def __init__(self, ID, sim, costs):
        self.myID = ID
        self.sim = sim

        self.routeTable = [0 for i in range(self.sim.NUM_NODES)] #Put 0 everywhere (no route yet)
        self.distanceTable = [[RouterSimulator.INFINITY] for i in range(self.sim.NUM_NODES)]#We know nothing for the moment so we put infinity for the cost of each node

        self.myGUI = GuiTextArea.GuiTextArea("  Output window for Router #" + str(ID) + "  ")
        self.costs = deepcopy(costs)

        # Initialize route table -> contains next hop
        for i in range(self.sim.NUM_NODES):
            if(self.costs[i] == self.sim.INFINITY):
                self.routeTable[i] = self.sim.INFINITY #We can't go here
            else:
                self.routeTable[i] = i

        # Initialize distance table
        for x in range(self.sim.NUM_NODES):
            for y in range(self.sim.NUM_NODES):
                if(x == y):
                    self.distanceTable[y][x] = 0 #distance from a thing to the same (de 1 à 1 par exemple)
                elif(y == self.myID):
                    self.distanceTable[self.myID][x] = self.costs[x] #distance between us and another thing : distance of the other thing for us
                else:
                    self.distanceTable[y][x] = self.sim.INFINITY #we don't know yet


        #Prevenir les autres (UpdateAll)

    # --------------------------------------------------
    def recvUpdate(self, pkt):
        #Update our distance table with latest from neighbour
        self.distanceTable[pkt.sourceid] = pkt.mincost
        #We can use another function to see if other nodes has changed something.
        if (self.check_for_changes()):
            self.Update_others()
        


    def check_for_changes(self):
        updateNeighbours = False
        #See and try to understand the bellmanFord function of the python example on github
        


    # --------------------------------------------------
    def sendUpdate(self, pkt):
        self.sim.toLayer2(pkt)
        #On gitHub, they didn't do any other thing on the java version
        
    def Update_others(self):
        newCosts = self.distanceTable[self.myID]
        for i in range(self.sim.NUM_NODES):
            if(i != self.myID): # Don't send to ourself 
                tmpPkt = RouterPacket.RouterPacket(self.myID, i, newCosts)#Id of the sender, if of the receiver, new cost
                self.sendUpdate(tmpPkt)

    # --------------------------------------------------
    def printDistanceTable(self):
        self.myGUI.println("Current table for " + str(self.myID) +
                           "  at time " + str(self.sim.getClocktime()))


        #Print distances
        self.myGUI.println("Distancetable:")
        
        #They uses a PrintTableStart() in the GitHub example : why ?

        for x in range(self.sim.NUM_NODES):
            self.myGUI.print(" Node" + str(x) + "    |\t\t")
            for y in range(self.sim.NUM_NODES):
                self.myGUI.print(str(self.distanceTable[x][y]) + "\t")
            self.myGUI.print('\n')
        self.myGUI.println("")

        # Print costs and routes
        self.myGUI.println("Costs and routes:")
        self.myGUI.print(" cost    |\t")
        for i in range(self.sim.NUM_NODES):
            self.myGUI.print('\t'+ str(self.costs[i]))
        self.myGUI.print("\n")
        self.myGUI.print(" route   |\t")
        for i in range(self.sim.NUM_NODES):
            self.myGUI.print('\t'+ str(self.routeTable[i]))
        self.myGUI.print("\n\n")

    # --------------------------------------------------
    def updateLinkCost(self, dest, newcost):
        pass


#They Created other functions : 
# Calculate cheapest
#broadcastUpdate

#Print neighbordistance table
#printourdistancetable
#printheader
