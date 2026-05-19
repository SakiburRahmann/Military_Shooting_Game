# Military Shooting Game (Lyra UE5 Fork)

This repository is a customized fork of Epic Games' Lyra Starter Game, featuring advanced Military FPS enhancements.

## Core Features & Customizations

### 🎥 True First-Person Camera Integration
A robust and immersive **True First-Person Camera** system has been fully integrated into the Lyra framework. 
Unlike standard virtual first-person setups, this system anchors the camera directly to the character's skeletal head socket. This allows full skeletal animations (including head-bob, landing impacts, and combat movements) to reflect directly in the first-person perspective.

#### Key Implementation Details:
* **State-Aware Pivot Location (`ULyraCameraMode`):** We added a helper method `ShouldUseFirstPersonCamera()` to evaluate whether the character is in a first-person state. The camera's dynamic pivot updates based on either the hardcoded class config or by querying the `ULyraHeroComponent` for the player's active view mode.
* **Aim Down Sights (ADS) Decoupling:** 
  * In **First-Person View**, scoping (ADS) seamlessly maintains the head-socket attachment and animates cleanly through the sights.
  * In **Third-Person View**, scoping continues using standard over-the-shoulder logic without snapping or zooming awkwardly into the character's head.
  * This is handled by conditionally bypassing third-person camera offsets in `ULyraCameraMode_ThirdPerson::UpdateView()` whenever the first-person view state is active.

## Setup Instructions

1. **Prerequisites:**
   * Unreal Engine 5.x
   * Visual Studio 2022 (with C++ Game Development workload)
   * Git LFS installed (`git lfs install`)

2. **Clone and Run:**
   * Clone this repository: `git clone https://github.com/SakiburRahmann/Military_Shooting_Game.git`
   * Generate Visual Studio project files (right-click `.uproject` -> Generate Project Files)
   * Build the solution in VS2022 and launch the editor.