#!/usr/bin/env python
import GuiTextArea
from RouterPacket import RouterPacket
from copy import deepcopy

INFINITY = 999


class RouterNode():
    myID = None
    myGUI = None
    sim = None
    costs = None
    routeTable = None  # Best known next-hop router for each destination node
    distanceTable = None
    neighbours = None

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
        # Initialize neighbours
        self.neighbours = []

        self.myGUI = GuiTextArea.GuiTextArea(
            "  Output window for Router #" + str(ID) + "  ")
        self.costs = deepcopy(costs)

        # Initialize route table -> contains next hop
        for i in range(self.sim.NUM_NODES):
            if (self.costs[i] == self.sim.INFINITY):
                self.routeTable[i] = self.sim.INFINITY  # We can't go here
            else:
                self.routeTable[i] = i
                if self.costs[i] != 0:
                    # neighbour
                    self.neighbours.append(i)

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

        self.printDistanceTable()
        self.updateNeighbours()

    # --------------------------------------------------

    def recvUpdate(self, packet):
        # Update our distance table with latest from neighbour
        self.distanceTable[packet.sourceid] = packet.mincost
        self.recalculateDistanceTable()

    def recalculateDistanceTable(self):
        tableChanged = False
        for destination in range(self.sim.NUM_NODES):
            minimum = INFINITY
            if destination == self.myID:
                continue
            for node in range(self.sim.NUM_NODES):
                cost = self.costs[node] + self.distanceTable[node][destination]
                if cost < minimum and node != self.myID:
                    minimum = cost
                    minimumNode = node
            if minimum != self.distanceTable[self.myID][destination]:
                self.distanceTable[self.myID][destination] = minimum
                if minimumNode in self.neighbours:
                    self.routeTable[destination] = minimumNode
                else:
                    self.routeTable[destination] = self.routeTable[minimumNode]
                tableChanged = True

        if tableChanged:
            self.updateNeighbours()

    # --------------------------------------------------

    def sendUpdate(self, packet):
        self.sim.toLayer2(packet)

    def updateNeighbours(self):
        newCosts = self.distanceTable[self.myID]
        for neighbour in self.neighbours:
            newCostsCopy = newCosts[:]

            if self.sim.POISONREVERSE:
                for routeDestination in range(self.sim.NUM_NODES):
                    if self.routeTable[routeDestination] == neighbour:
                        newCostsCopy[routeDestination] = INFINITY

            # Id of the sender, id of the receiver, new cost
            packet = RouterPacket(self.myID, neighbour, newCostsCopy)
            self.sendUpdate(packet)

    # --------------------------------------------------
    def printDistanceTable(self):
        self.myGUI.println("Current table for " + str(self.myID) +
                           "  at time " + str(self.sim.getClocktime()))

        # Print distances
        self.myGUI.println("Distancetable:")

        for x in range(self.sim.NUM_NODES):
            self.myGUI.print(" Node" + str(x) + "    |\t\t")
            for y in range(self.sim.NUM_NODES):
                self.myGUI.print(str(self.distanceTable[x][y]) + "\t")
            self.myGUI.print('\n')
        self.myGUI.println("")

        # Print costs and routes
        self.myGUI.println("Costs and routes:")
        self.myGUI.print(" linkcost|\t")
        for i in range(self.sim.NUM_NODES):
            self.myGUI.print('\t' + str(self.costs[i]))
        self.myGUI.print("\n")
        self.myGUI.print(" cost    |\t")
        for i in range(self.sim.NUM_NODES):
            self.myGUI.print('\t' + str(self.distanceTable[self.myID][i]))
        self.myGUI.print("\n")
        self.myGUI.print(" route   |\t")
        for i in range(self.sim.NUM_NODES):
            self.myGUI.print('\t' + str(self.routeTable[i]))
        self.myGUI.print("\n\n")

    # --------------------------------------------------
    def updateLinkCost(self, destination, newcost):
        print("Node " + str(self.myID) + "newcost" + str(newcost))
        self.costs[destination] = newcost

        self.recalculateDistanceTable()
