#!/usr/bin/env python
import GuiTextArea
from RouterPacket import RouterPacket
import F
from copy import deepcopy

INFINITY = 999


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
        self.routeTable = [0 for _ in range(self.sim.NUM_NODES)]
        # We don't know anything yet so we set the cost of each node to infty
        self.distanceTable = [[INFINITY for _ in range(self.sim.NUM_NODES)]
                              for _ in range(self.sim.NUM_NODES)]

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
        for destination in range(self.sim.NUM_NODES):
            for source in range(self.sim.NUM_NODES):
                if (destination == source):
                    # distance from this node to itself (de 1 à 1 par exemple)
                    self.distanceTable[source][destination] = 0
                elif (source == self.myID):
                    # distance between this node and destination node
                    self.distanceTable[self.myID][destination] = self.costs[destination]
                else:
                    # we don't know the distance yet
                    self.distanceTable[source][destination] = self.sim.INFINITY

        # Prevenir les autres (UpdateAll)
        self.printDistanceTable()
        self.updateNeighbours()

    # --------------------------------------------------

    def recvUpdate(self, packet):
        # Update our distance table with latest from neighbour
        self.distanceTable[packet.sourceid] = packet.mincost
        self.recalculateDistanceTable()

    def recalculateDistanceTable(self):
        tableChanged = False
        # See and try to understand the bellmanFord function of the python example on github
        for destination in range(self.sim.NUM_NODES):
            minimum = INFINITY
            for node in range(self.sim.NUM_NODES):
                cost = self.costs[node] + self.distanceTable[node][destination]
                if cost < minimum:
                    minimum = cost
                    minimumNode = node
            if minimum != self.distanceTable[self.myID][destination]:
                self.distanceTable[self.myID][destination] = minimum
                self.routeTable[destination] = minimumNode
                tableChanged = True

        if tableChanged:
            self.updateNeighbours()

    # --------------------------------------------------

    def sendUpdate(self, packet):
        self.sim.toLayer2(packet)

    def updateNeighbours(self):
        newCosts = self.distanceTable[self.myID]
        for node in range(self.sim.NUM_NODES):
            if (node != self.myID and self.costs[node] != INFINITY):
                # Send only to neighbours
                neighbour = node
                # Id of the sender, id of the receiver, new cost
                packet = RouterPacket(self.myID, neighbour, newCosts)
                self.sendUpdate(packet)

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
