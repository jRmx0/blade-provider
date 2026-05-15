# C∗ Algorithm: Coverage Path Planning for Unknown Environments

> **Source:** *C∗: A Coverage Path Planning Algorithm for Unknown Environments using Rapidly Covering Graphs*  
> **Authors:** Zongyuan Shen, James P. Wilson, Shalabh Gupta — University of Connecticut  
> **arXiv:** 2505.13782v1 [cs.RO] 20 May 2025

---

## Table of Contents

- [C∗ Algorithm: Coverage Path Planning for Unknown Environments](#c-algorithm-coverage-path-planning-for-unknown-environments)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
    - [Core Properties](#core-properties)
    - [Key Advantages over Grid-based Methods](#key-advantages-over-grid-based-methods)
  - [Problem Description](#problem-description)
    - [Robot Model](#robot-model)
    - [Environment Model](#environment-model)
    - [Definitions](#definitions)
    - [Objective](#objective)
  - [Algorithm Summary](#algorithm-summary)
    - [Iteration Structure](#iteration-structure)
  - [Data Structures](#data-structures)
    - [Rapidly Covering Graph (RCG)](#rapidly-covering-graph-rcg)
      - [RCG Properties](#rcg-properties)
      - [State Encoding](#state-encoding)
    - [Sampling Front](#sampling-front)
    - [Frontier Samples \& Laps](#frontier-samples--laps)
      - [Lap Generation Rules](#lap-generation-rules)
      - [Sample Placement on Laps](#sample-placement-on-laps)
  - [Algorithm Steps](#algorithm-steps)
    - [Step 1: Navigation, Discovery \& Coverage](#step-1-navigation-discovery--coverage)
    - [Step 2: Progressive Sampling](#step-2-progressive-sampling)
    - [Step 3: Progressive RCG Growth](#step-3-progressive-rcg-growth)
      - [3a. RCG Expansion](#3a-rcg-expansion)
      - [3b. RCG Pruning](#3b-rcg-pruning)
    - [Step 4: Coverage Trajectory Generation](#step-4-coverage-trajectory-generation)
  - [Sub-strategies](#sub-strategies)
    - [Goal Node Selection](#goal-node-selection)
    - [State Update](#state-update)
    - [Dead-end Escape](#dead-end-escape)
    - [Coverage Hole Prevention](#coverage-hole-prevention)
      - [Detection of Potential Coverage Holes](#detection-of-potential-coverage-holes)
      - [TSP Setup](#tsp-setup)
      - [TSP Solving](#tsp-solving)
  - [Pseudocode](#pseudocode)
  - [Key Definitions Reference](#key-definitions-reference)
  - [Implementation Notes](#implementation-notes)
    - [Parameters](#parameters)
    - [Coordinate Frame](#coordinate-frame)
    - [Key Implementation Details](#key-implementation-details)
    - [RCG Invariants to Maintain](#rcg-invariants-to-maintain)
    - [Coverage Hole Detection Edge Cases](#coverage-hole-detection-edge-cases)

---

## Overview

C∗ is a **sample-based, online (real-time) Coverage Path Planning (CPP) algorithm** for robots operating in **unknown environments**. It is built on a novel data structure called the **Rapidly Covering Graph (RCG)**.

### Core Properties

| Property | Description |
|----------|-------------|
| **Type** | Sample-based (not grid-based) |
| **Environment** | Unknown; discovered incrementally via onboard sensors |
| **Coverage Pattern** | Back-and-forth with adaptive TSP-optimized coverage of coverage holes |
| **Waypoint Selection** | Non-myopic (due to sparse RCG structure) |
| **Dead-end Handling** | Retreat node concept + A∗ shortest path |
| **Coverage Holes** | Detected in situ; covered immediately via TSP trajectory |
| **Coverage Guarantee** | Analytically proven complete coverage |

### Key Advantages over Grid-based Methods

- No cellular decomposition required
- Non-myopic waypoint generation (vs. myopic grid neighbors)
- Proactive in-situ coverage hole prevention (unique to C∗)
- Computationally efficient: O(|S_Fi|) per iteration
- Memory efficient: O(|N_i|) space

---

## Problem Description

### Robot Model

Robot **R** is equipped with:
- **Localization device**: GPS, IMU, wheel encoder, Indoor Localization System, or SLAM
- **Range detector / mapping sensor** (e.g., lidar, ultrasonics): range $r_d \in \mathbb{R}^+$, detects obstacles within FOV and maps the environment
- **Coverage device/sensor** (e.g., cleaning brush): range $r_c \in \mathbb{R}^+$, performs the coverage task

### Environment Model

- $\mathcal{A} \subset \mathbb{R}^2$ — unknown area populated by obstacles of arbitrary shapes
- $\mathcal{A}_o \subset \mathcal{A}$ — space occupied by obstacles
- $\mathcal{A}_f = \mathcal{A} \setminus \mathcal{A}_o$ — **obstacle-free coverage space** (connected; every point reachable from every other)

### Definitions

**Definition: Coverage Trajectory**

$$\Lambda_T \triangleq [\lambda(t)]_{t=0}^{T}$$

where $\lambda : [0,T] \to \mathcal{A}_f$ maps time to robot position, $\lambda(0)$ is start, $\lambda(T)$ is end.

**Definition: Complete Coverage**

Let $\mathcal{A}_c(\lambda(t)) \subset \mathcal{A}_f$ be the area covered by the coverage device at point $\lambda(t)$. Coverage of $\mathcal{A}_f$ is **complete** if:

$$\mathcal{A}_f \subseteq \bigcup_{t \in [0,T]} \mathcal{A}_c(\lambda(t))$$

### Objective

Generate $\Lambda_T$ such that:
1. Complete coverage of $\mathcal{A}_f$ is achieved
2. Robot does not get stuck in dead-ends
3. Back-and-forth coverage pattern is produced
4. Coverage holes are prevented (no long return trajectories needed later)
5. Coverage time, trajectory length, turns, and overlaps are minimized

---

## Algorithm Summary

C∗ generates the coverage trajectory **incrementally** during robot navigation. Each iteration performs four steps:

```
while open nodes remain:
  1. Navigate to current waypoint, sense environment, cover region
  2. Progressive sampling of newly discovered area → frontier samples
  3. Progressive RCG growth (expand + prune) using frontier samples
  4. Coverage trajectory generation: select next waypoint from updated RCG
```

### Iteration Structure

- Iterations indexed by $i \in \{0, 1, \ldots, m\}$
- Total iterations: $m + 1$
- **Iteration 0**: Robot at start $p_0$; senses environment; creates first RCG portion; outputs $p_1$
- **Iteration $i$ ($1 \le i \le m-1$)**: Robot moves $p_{i-1} \to p_i$; maps environment; grows RCG; outputs $p_{i+1}$
- **Iteration $m$**: Robot moves $p_{m-1} \to p_m$; no new area; coverage complete

**Definition: Waypoint Sequence**

$$\mathcal{P} = \{p_0, p_1, \ldots, p_m\}$$

where $p_i \in \mathcal{A}_f$ are intermediate goal points, $p_i \neq p_j\ \forall i \neq j$ (no loops).

**Definition: Node Sequence**

$$\hat{\mathcal{N}} = \{\hat{n}_0, \hat{n}_1, \ldots, \hat{n}_m\}$$

One-to-one correspondence: $p_i = \text{pos}(\hat{n}_i)$.

---

## Data Structures

### Rapidly Covering Graph (RCG)

**Definition:** An RCG $\mathcal{G} = (\mathcal{N}, \mathcal{E})$ is a **simple, connected, and planar graph** where:

- $\mathcal{N} = \{n_j : j = 1, \ldots |\mathcal{N}|\}$ — **node set**: each node corresponds to a frontier sample and represents a potential waypoint on the coverage trajectory
- $\mathcal{E} = \{e(n_j, n_k)\}$ — **edge set**: each edge connects two nodes and represents a collision-free traversal path

**Neighbor:** $n' \in \mathcal{N}$ is a neighbor of $n \in \mathcal{N}$ iff $\exists\, e(n, n') \in \mathcal{E}$.

**Neighborhood:** $\text{Nb}(n) \subset \mathcal{N}$ — set of all neighbors of $n$.

#### RCG Properties

| ID | Property | Description |
|----|----------|-------------|
| P1 | **Simple** | No more than one edge between any two nodes; no self-loops |
| P2 | **Planar** | Embeddable in a plane with no edge crossings |
| P3 | **Connected** | Any node reachable from any other via edges |
| P4 | **Essential** | Minimum sufficient graph — only essential nodes and edges |
| P5 | **Sparse** | Nodes only near obstacles and unknown area |
| P6 | **Scalable** | $|\mathcal{N}| + |\mathcal{E}| \le 4|\mathcal{N}| - 6$ (from Euler's formula: $|\mathcal{E}| \le 3|\mathcal{N}| - 6$) |

#### State Encoding

Each node $n \in \mathcal{N}$ has a state $q(n) \in \{\text{Cl}, \text{Op}\}$:
- **Cl (Closed)**: visited by the robot
- **Op (Open)**: unvisited

Sets at iteration $i$:
- $\mathcal{N}^{Op}_i$ — Open nodes
- $\mathcal{N}^{Cl}_i$ — Closed nodes
- $\mathcal{N}^{Op}_i \cup \mathcal{N}^{Cl}_i = \mathcal{N}_i$

---

### Sampling Front

**Definition:** The sampling front $\mathcal{F}_i \subseteq \mathcal{A}_{d,i}$ ($i \in \{0, \ldots, m-1\}$) is the **obstacle-free and unsampled** portion of the area $\mathcal{A}_{d,i}$ discovered in iteration $i$.

- $\mathcal{A}_{d,i} = \bigcup_{t \in [t_{i-1}, t_i]} \mathcal{A}_d(\lambda(t))$ — total area discovered in iteration $i$
- $\mathcal{A}'_d = \bigcup_i \mathcal{A}_{d,i}$ — total area discovered so far
- $\mathcal{A}_u = \mathcal{A} \setminus \mathcal{A}'_d$ — still unknown area
- $\mathcal{F}_i$ contains **only newly discovered and unsampled** area (previously sampled areas excluded)

---

### Frontier Samples & Laps

**Definition: Frontier Sample**

A sample $s \in \mathcal{S}$ is a **frontier sample** if it is adjacent to:
- the **unknown area**, and/or  
- an **obstacle or boundary**

Formally: ball $\mathcal{B}(s, w)$ of radius $w$ centered at $s$ contains unknown and/or obstacle area.

**Definition: Lap**

A **lap** is a virtual straight line segment in the coverage space whose two ends terminate either at an obstacle or the coverage space boundary.

#### Lap Generation Rules

- Laps are parallel to each other
- Distance between adjacent laps = sampling resolution $w \in \mathbb{R}^+$
- Laps are drawn **along the direction of desired back-and-forth motion** (vertical axis parallel to laps)
- First lap drawn in $\mathcal{F}_0$ starting from $p_0$
- Laps are **consistent**: laps separated by an obstacle are aligned on the same line whenever possible
- Only portions within $\mathcal{F}_i$ are created (ends may not reach obstacle/boundary)

#### Sample Placement on Laps

- First sample: at $p_0$ in $\mathcal{F}_0$ at iteration $i = 0$
- All laps in $\mathcal{F}_0$ populated at iteration 0
- At iteration $i$: samples placed on each lap in $\mathcal{F}_i$
- Spacing between adjacent samples on a lap: $\delta w$, where $\delta \in \mathbb{N}^+$, $\delta \ge 1$ is the **minimum multiplier** that allows placement of a frontier sample
- Multiple frontier samples possible on one lap depending on obstacle geometry

The progressive sampling produces **nonuniform and sparse** samples → computational/memory efficiency + non-myopic waypoint selection.

---

## Algorithm Steps

### Step 1: Navigation, Discovery & Coverage

The robot moves from $p_{i-1}$ to $p_i$, simultaneously:
- **Covering** the region using its coverage device/sensor
- **Sensing** the environment using the range detector
- **Mapping** newly detected obstacles

Total area discovered and mapped in iteration $i$:

$$\mathcal{A}_{d,i} = \bigcup_{t \in [t_{i-1}, t_i]} \mathcal{A}_d(\lambda(t))$$

At iteration $m$: no new area discovered (all previously mapped).

---

### Step 2: Progressive Sampling

Upon reaching $p_i$:
1. **Create sampling front** $\mathcal{F}_i$ from $\mathcal{A}_{d,i}$ (exclude already-sampled areas)
2. **Draw laps** in $\mathcal{F}_i$ parallel to back-and-forth direction, spaced $w$ apart
3. **Place frontier samples** $\mathcal{S}_i$ on laps: spacing $\delta w$, only at positions adjacent to unknown/obstacle area

**Sampling resolution parameter:** $w \in \mathbb{R}^+$

**Coverage overlap condition:** $w \le 2r_c$ ensures the region between adjacent laps is fully covered.

---

### Step 3: Progressive RCG Growth

At each iteration $i$, the RCG $\mathcal{G}_i = (\mathcal{N}_i, \mathcal{E}_i)$ is grown via:
1. **RCG Expansion** into $\mathcal{F}_i$
2. **RCG Pruning** to maintain sparse structure

#### 3a. RCG Expansion

1. Assign all frontier samples $\mathcal{S}_i$ as new nodes $\mathcal{N}_{new}$, where $|\mathcal{N}_{new}| = |\mathcal{S}_i|$
2. Add to existing node set: $\mathcal{N}'_{i-1} = \mathcal{N}_{i-1} \cup \mathcal{N}_{new}$
3. For each new node $n \in \mathcal{N}_{new}$ on each lap, create edges to:
   - **i.** Adjacent nodes on the **same lap** (above and below)
   - **ii.** All nodes within distance $\sqrt{2}w$ on each **adjacent lap** (left and right)
4. Each edge checked for feasibility: must lie in $\mathcal{A}_f$ (collision-free)
5. Add to existing edge set: $\mathcal{E}'_{i-1} = \mathcal{E}_{i-1} \cup \mathcal{E}_{new}$
6. Expanded graph: $\mathcal{G}'_{i-1} = \{\mathcal{N}'_{i-1}, \mathcal{E}'_{i-1}\}$

#### 3b. RCG Pruning

**Essential Node** (Definition III.8): A node $n \in \mathcal{N}$ is essential if:
1. Adjacent to the **unknown area**, OR
2. An **end node of a lap**, OR
3. **Not** an end-node but connected to an end node $n_x \in \mathcal{N}$ of an adjacent lap ($n \in \text{Nb}(n_x)$), AND:
   - a. $n_x$ has no other neighbor on $n$'s lap, OR
   - b. $n_x$ has other neighbors on $n$'s lap which are all non-end nodes, but edge $e(n, n_x)$ is **closest to an obstacle or unknown area**

```
Node n
├── Is n adjacent to A_u (unknown)?  → YES → Essential
├── Is n an end node of its lap?      → YES → Essential
├── Is n ∈ Nb(n_x) for end node n_x of adjacent lap?
│   ├── YES → Is n the only neighbor of n_x at its lap?  → YES → Essential
│   │         Is edge (n, n_x) closer to obstacle/unknown? → YES → Essential
│   │         Otherwise → Inessential
│   └── NO → Inessential
```

**Node Pruning Process:**
- Let $\mathcal{N}_{\partial\mathcal{F}_i} \subseteq \mathcal{N}_{i-1}$ = previous nodes adjacent to boundary $\partial\mathcal{F}_i$
- Check each $n \in \{\mathcal{N}_{new} \cup \mathcal{N}_{\partial\mathcal{F}_i}\}$ for essentialness
- Prune inessential set $\mathcal{N}_{iness}$

**Essential Edge** (Definition III.9): An edge $e(n_x, n_y) \in \mathcal{E}$ is essential if $n_x$ and $n_y$ are both essential AND:
1. $n_x$ and $n_y$ are on the **same lap**, OR
2. $n_x$ and $n_y$ are on **adjacent laps**, AND:
   - a. Both are **end nodes** of laps, OR
   - b. $n_x$ is an end-node and $n_y$ is a non-end node, AND:
     - i. $n_x$ has no other neighbor on $n_y$'s lap, OR
     - ii. $n_x$ has other neighbors on $n_y$'s lap which are all non-end nodes, but $e(n_x, n_y)$ is closest to an obstacle or unknown area

**Edge Pruning Process:**
- For each pruned node $n \in \mathcal{N}_{iness}$:
  - Edges to two adjacent nodes on same lap → **merged into one edge**
  - Edges to nodes on adjacent laps → **pruned**
- Additional inessential edges between essential nodes are also pruned

Result: Updated sparse RCG $\mathcal{G}_i = (\mathcal{N}_i, \mathcal{E}_i)$.

---

### Step 4: Coverage Trajectory Generation

Once RCG is updated, select next waypoint $p_{i+1}$:
1. All new nodes in $\mathcal{G}_i$ assigned state **Op**
2. Select goal node $\hat{n}_{i+1}$ using **Goal Node Selection Strategy**
3. Update state of $\hat{n}_i$ using **State Update Strategy**
4. Detect and handle **Coverage Holes**
5. Set $p_{i+1} = \text{pos}(\hat{n}_{i+1})$

---

## Sub-strategies

### Goal Node Selection

Neighbors of current node $\hat{n}_i$ by direction:
- $\hat{n}^L_i$ — left lap neighbor
- $\hat{n}^U_i$ — same lap, upward
- $\hat{n}^D_i$ — same lap, downward
- $\hat{n}^R_i$ — right lap neighbor

**Priority order** for selecting $\hat{n}_{i+1}$ (first Open neighbor wins):

$$\hat{n}^L_i \to \hat{n}^U_i \to \hat{n}^D_i \to \hat{n}^R_i$$

- If multiple Open neighbors exist on left/right adjacent lap → **random pick**
- If **no Open neighbor** found → **dead-end** (see Dead-end Escape)

This generates back-and-forth pattern: robot goes to leftmost lap with open neighbor, traverses until end, shifts right.

```
Algorithm 1: SelectGoalNode
  if q(n̂_L_i) = Op  → n̂_{i+1} = n̂_L_i   // go left
  else if q(n̂_U_i) = Op  → n̂_{i+1} = n̂_U_i   // go up
  else if q(n̂_D_i) = Op  → n̂_{i+1} = n̂_D_i   // go down
  else if q(n̂_R_i) = Op  → n̂_{i+1} = n̂_R_i   // go right
  else  // dead-end
    if N_retreat ≠ ∅  → n̂_{i+1} ← EscapeDeadEnd(n̂_i, N_retreat)
    else  → Coverage complete
  p_{i+1} = pos(n̂_{i+1})
```

---

### State Update

```
Algorithm 2: UpdateState
  if q(n̂_U_i) = Cl OR q(n̂_D_i) = Cl:
    q(n̂_i) = Cl  // close node
    N_Op_i = N_Op_i \ {n̂_i}
    N_Cl_i = N_Cl_i ∪ {n̂_i}
    
    if n̂_{i+1} = n̂_L_i:   // goal selected on left lap
      if q(n̂_U_i) = Op:
        create LINK NODE above n̂_i at distance w
      if q(n̂_D_i) = Op:
        create LINK NODE below n̂_i at distance w
```

**Key rule:** Node $\hat{n}_i$ is kept **Op** only when **both** $\hat{n}^U_i$ and $\hat{n}^D_i$ are Op (to avoid breaking back-and-forth trajectory in the middle of open nodes on a lap).

**Link nodes:** Created when robot moves left but leaves an uncovered portion of the lap above/below. Link nodes are pruned after being covered and closed.

---

### Dead-end Escape

**Definition: Dead-end**

A dead-end occurs at $\hat{n}_i$ when:
$$q(\hat{n}^L_i) = q(\hat{n}^U_i) = q(\hat{n}^D_i) = q(\hat{n}^R_i) = \text{Cl}$$

No Open neighbor exists in $\text{Nb}(\hat{n}_i)$.

**Definition: Retreat Node**

A node $n \in \mathcal{N}^{Op}_i$ is a **retreat node** if it is adjacent to the robot's trajectory (within distance $\sqrt{2}w$ from the robot).

**Retreat Node Set $\mathcal{N}_{retreat}$:** Constantly updated during navigation:
- **Add**: any Open node within $\sqrt{2}w$ from robot
- **Remove**: any Closed node

Note: There are always retreat nodes until coverage is complete (because the area is connected).

**Dead-end Escape Procedure:**
1. Find nearest retreat node in $\mathcal{N}_{retreat}$ (minimum A∗ path cost)
2. Compute shortest path to it using **A∗ algorithm**
3. Move robot to nearest retreat node
4. Resume back-and-forth coverage

---

### Coverage Hole Prevention

**Definition: Coverage Hole**

An obstacle-free uncovered region is a **coverage hole** if surrounded by:
- a) obstacles, AND
- b) covered regions

#### Detection of Potential Coverage Holes

When robot reaches $\hat{n}_i$ and selects $\hat{n}_{i+1}$:
1. Pick an Open node in neighborhood of $\hat{n}_i$, assign index $\alpha = 1$
2. **Flood-fill** from this node: recursively label all reachable Open neighbors with index $\alpha$
3. Stop at Closed nodes or goal node $\hat{n}_{i+1}$
4. Check: if any labeled node is adjacent to unknown area → **disqualified** (not a coverage hole)
5. If not adjacent to unknown → **coverage hole detected** → node set $\mathcal{N}^\alpha_i$
6. Repeat for any unlabeled Open neighbor of $\hat{n}_i$ with index $\alpha = 2$, etc.
7. $\mathcal{N}^{CH}_i = \bigcup_\alpha \mathcal{N}^\alpha_i$ — all coverage holes around $\hat{n}_i$

#### TSP Setup

Once $\mathcal{N}^{CH}_i$ is found:
1. Add nodes on each lap **inside** the coverage hole with inter-node distance $w$
2. Form fully connected sub-graph $\bar{\mathcal{G}}^{CH}_i = (\bar{\mathcal{N}}^{CH}_i, \bar{\mathcal{E}}^{CH}_i)$
3. Each edge in $\bar{\mathcal{E}}^{CH}_i$ assigned **A∗ shortest path cost** between its endpoints
4. $\bar{\mathcal{N}}^{CH}_i$ includes: current node $\hat{n}_i$, goal node $\hat{n}_{i+1}$, coverage hole nodes, appended nodes

**TSP Start/End node determination:**

```
Algorithm 3: ComputeTSPTrajectory
  n_s^TSP = n̂_i  // always start at current node
  
  if (∃n ∈ Nb(n̂_{i+1}) s.t. q_i(n) = Op and n ∉ N̄^CH_i)
     OR n̂_{i+1} adjacent to unknown area:
    n_e^TSP = n̂_{i+1}   // end at goal (merges into main trajectory)
    
  else if ∃n ∈ Nb(n̂_i) s.t. q_i(n) = Op and n ∉ N̄^CH_i:
    n_e^TSP = n̂_i        // end at current node
    
  else:
    n_e^TSP = unspecified
```

#### TSP Solving

Since TSP is NP-hard, a heuristic approach:
1. **Nearest Neighbor Algorithm** → initial node sequence — $O(|\bar{\mathcal{N}}^{CH}_i|^2)$
2. **2-opt Algorithm** → improve solution — $O(|\bar{\mathcal{N}}^{CH}_i|)$

**Dummy node $\eta$** added for open-loop TSP cases (when $n_e^{TSP} \neq n_s^{TSP}$):
- $u_{\eta, \hat{n}_i} = 0$ (free to start here)
- $u_{\eta, \hat{n}_{i+1}} = 0$ (if Condition 1) or $\infty$ (if Condition 3)
- $u_{\eta, n} = \infty$ for all other nodes
- $\eta$ removed from sequence after optimization

For Condition 2 ($n_e^{TSP} = n_s^{TSP}$): closed-loop TSP, no dummy node needed.

After TSP execution: $\hat{n}_i \leftarrow n_e^{TSP}$; $\hat{n}_{i+1} \leftarrow n_e^{TSP}$; resume back-and-forth.

---

## Pseudocode

```
Algorithm 4: C* Execution

Input: starting location p_{-1} = p_0, initial RCG G_{-1} = ∅

i ← 0
while N_Op_{i-1} ≠ ∅ do:
  
  // Step 1: Navigate, sense, cover
  A_{d,i} ← NavigateSense(p_{i-1}, p_i)
  
  // Step 2: Progressive sampling
  F_i ← CreateSamplingFront(A_{d,i})
  S_i ← GenerateFrontierSamples(F_i)
  
  // Step 3: Progressive RCG growth
  G'_{i-1} ← ExpandRCG(G_{i-1}, S_i)
  G_i ← PruneRCG(G'_{i-1})
  
  // Step 4: Trajectory generation
  (n̂_{i+1}, p_{i+1}) ← SelectGoalNode(G_i, n̂_i)      // Alg. 1
  (q_i(n̂_i), N_Op_i, N_Cl_i) ← UpdateState(...)       // Alg. 2
  
  // Coverage hole check
  N^CH_i ← DetectCoverageHoles(G_i, n̂_i, n̂_{i+1})
  if N^CH_i ≠ ∅:
    Λ^TSP_i ← ComputeTSPTrajectory(N^CH_i, n̂_i, n̂_{i+1})   // Alg. 3
    n_e^TSP ← ExecuteTSPTrajectory(Λ^TSP_i)
    n̂_i ← n_e^TSP
    n̂_{i+1} ← n_e^TSP
  
  i ← i + 1
end
```

---


## Key Definitions Reference

| Term | Definition |
|------|-----------|
| $\mathcal{A}$ | Unknown area (full environment) |
| $\mathcal{A}_o$ | Obstacle-occupied space |
| $\mathcal{A}_f = \mathcal{A} \setminus \mathcal{A}_o$ | Obstacle-free coverage space |
| $\mathcal{A}_{d,i}$ | Area discovered in iteration $i$ |
| $\mathcal{A}'_d$ | Total area discovered so far |
| $\mathcal{A}_u = \mathcal{A} \setminus \mathcal{A}'_d$ | Unknown remaining area |
| $r_d$ | Range detector range |
| $r_c$ | Coverage device range |
| $w$ | Sampling resolution (lap spacing) |
| $\delta$ | Minimum multiplier for frontier sample spacing ($\delta w$) |
| $\mathcal{F}_i$ | Sampling front at iteration $i$ |
| $\mathcal{S}_i$ | Frontier samples at iteration $i$ |
| $\mathcal{G}_i = (\mathcal{N}_i, \mathcal{E}_i)$ | RCG at iteration $i$ |
| $\mathcal{N}_{new}$ | Newly added nodes at current iteration |
| $\mathcal{N}_{\partial\mathcal{F}_i}$ | Previous nodes adjacent to $\partial\mathcal{F}_i$ |
| $\mathcal{N}_{iness}$ | Inessential nodes to be pruned |
| $\mathcal{N}^{Op}_i$ | Open (unvisited) nodes |
| $\mathcal{N}^{Cl}_i$ | Closed (visited) nodes |
| $\mathcal{N}_{retreat}$ | Retreat nodes (Open, adjacent to robot trajectory) |
| $\mathcal{N}^{CH}_i$ | All coverage hole nodes near $\hat{n}_i$ |
| $\hat{n}_i$ | Current goal node at iteration $i$ |
| $\hat{n}^L_i, \hat{n}^U_i, \hat{n}^D_i, \hat{n}^R_i$ | Neighbors in left/up/down/right directions |
| $q(n)$ | State encoding: $\text{Cl}$ or $\text{Op}$ |
| $\Lambda_T$ | Coverage trajectory |
| $\Lambda^{TSP}_i$ | TSP trajectory for coverage hole |
| $\mathcal{P} = \{p_0, \ldots, p_m\}$ | Waypoint sequence |
| $\hat{\mathcal{N}} = \{\hat{n}_0, \ldots, \hat{n}_m\}$ | Node sequence |



---

## Implementation Notes

### Parameters

| Parameter | Description | Constraint |
|-----------|-------------|-----------|
| $w$ | Sampling resolution / lap spacing | $w \le 2r_c$ for complete coverage between laps |
| $r_d$ | Range detector range | Determines sampling front size |
| $r_c$ | Coverage device range | Determines required $w$ |
| $\delta$ | Frontier sample spacing multiplier | $\delta \ge 1$, integer |

### Coordinate Frame

- Vertical axis is **parallel to the laps**
- Directions (left/up/down/right) are defined relative to this fixed coordinate frame
- "Left" = toward the leftmost unvisited lap; "Right" = away from covered laps

### Key Implementation Details

1. **Laps are virtual** — not stored as geometric objects, used only for sampling and node organization

2. **Edge feasibility check** — when creating edges during expansion, verify the edge lies entirely in $\mathcal{A}_f$ (no obstacle crossing)

3. **Retreat node set** — maintain as a dynamic set updated every step; add Open nodes within $\sqrt{2}w$, remove Closed nodes

4. **Link nodes** — created when a node is closed while adjacent lap portions remain open; pruned after coverage

5. **Coverage hole detection triggers** — runs at every iteration after goal node selection; uses flood-fill starting from Open neighbors of $\hat{n}_i$

6. **TSP dummy node trick** — used for open-loop TSP to enforce specific start/end nodes via $\infty$-cost edges

7. **A∗ for retreat/TSP** — standard A∗ on the RCG graph; edge costs = Euclidean distances or A∗ shortest path costs

8. **Simulation parameters used in paper:**
   - Environment: $50m \times 50m$
   - Laser FOV: $360°$, range $15m$
   - Robot speed: $0.5 \text{m/s}$, max acceleration: $0.5 \text{m/s}^2$, min turning radius: $0.04m$
   - Sampling resolution: $w = 1m$
   - Implementation language: C++
   - Hardware: 2.60 GHz CPU, 32 GB RAM

### RCG Invariants to Maintain

- Simple: no duplicate edges or self-loops
- Planar: edges only connect same-lap adjacent nodes and cross-lap end nodes (no crossings)
- Connected: always maintain connections between adjacent laps
- After every prune: only essential nodes/edges remain

### Coverage Hole Detection Edge Cases

- A region is **not** a coverage hole if any node in it is adjacent to unknown area (the hole may grow)
- Multiple coverage holes can be adjacent to $\hat{n}_i$ simultaneously (indexed separately)
- Goal node $\hat{n}_{i+1}$ is used as boundary during flood-fill (stop condition)
