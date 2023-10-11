#!/usr/bin/env python
import GuiTextArea
import RouterPacket
import F
import RouterSimulator
from copy import deepcopy


class RouterNode():
    myID = None
    myGUI = None
    sim = None
    costs = None
    routeTable = None  # Best known next-hop router for each destination node
    distanceTable = None

    # Access simulator variables with:
    # self.sim.POISONREVERSE, self.sim.NUM_NODES, etc.

    # --------------------------------------------------
    def __init__(self, ID, sim, costs):
        self.myID = ID
        self.sim = sim

        # Put 0 everywhere (no route yet)
        self.routeTable = [0 for i in range(self.sim.NUM_NODES)]
        # We don't know anything yet so we set the cost of each node to infty
        self.distanceTable = [[RouterSimulator.INFINITY]
                              for i in range(self.sim.NUM_NODES)]

        self.myGUI = GuiTextArea.GuiTextArea(
            "  Output window for Router #" + str(ID) + "  ")
        self.costs = deepcopy(costs)

        # Initialize route table -> contains next hop
        for i in range(self.sim.NUM_NODES):
            if (self.costs[i] == self.sim.INFINITY):
                self.routeTable[i] = self.sim.INFINITY  # We can't go here
            else:
                self.routeTable[i] = i

        # Initialize distance table
        for x in range(self.sim.NUM_NODES):
            for y in range(self.sim.NUM_NODES):
                if (x == y):
                    # distance from a thing to the same (de 1 à 1 par exemple)
                    self.distanceTable[y][x] = 0
                elif (y == self.myID):
                    # distance between this node and node x
                    self.distanceTable[self.myID][x] = self.costs[x]
                else:
                    # we don't know yet
                    self.distanceTable[y][x] = self.sim.INFINITY

        # Prevenir les autres (UpdateAll)

    # --------------------------------------------------

    def recvUpdate(self, pkt):
        # Update our distance table with latest from neighbour
        self.distanceTable[pkt.sourceid] = pkt.mincost
        # We can use another function to see if other nodes have updated
        if (self.check_for_changes()):
            self.Update_others()

    def check_for_changes(self):
        updateNeighbours = False
        # See and try to understand the bellmanFord function of the python example on github

    # --------------------------------------------------

    def sendUpdate(self, pkt):
        self.sim.toLayer2(pkt)
        # On gitHub, they didn't do any other thing on the java version

    def Update_others(self):
        newCosts = self.distanceTable[self.myID]
        for i in range(self.sim.NUM_NODES):
            if (i != self.myID):  # Don't send to ourself
                # Id of the sender, if of the receiver, new cost
                tmpPkt = RouterPacket.RouterPacket(self.myID, i, newCosts)
                self.sendUpdate(tmpPkt)

    # --------------------------------------------------
    def printDistanceTable(self):
        self.myGUI.println("Current table for " + str(self.myID) +
                           "  at time " + str(self.sim.getClocktime()))

        # Print distances
        self.myGUI.println("Distancetable:")

        # They uses a PrintTableStart() in the GitHub example : why ?

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
            self.myGUI.print('\t' + str(self.costs[i]))
        self.myGUI.print("\n")
        self.myGUI.print(" route   |\t")
        for i in range(self.sim.NUM_NODES):
            self.myGUI.print('\t' + str(self.routeTable[i]))
        self.myGUI.print("\n\n")

    # --------------------------------------------------
    def updateLinkCost(self, dest, newcost):
        pass


# They Created other functions :
# Calculate cheapest
# broadcastUpdate

# Print neighbordistance table
# printourdistancetable
# printheader
