# Vortex UI Skeleton

Minimal Vite + React + TS app with:
- Splash → Hub → Editor routes
- Docking layout via react-mosaic-component
- Node graph via @xyflow/react (ReactFlow)
- Tailwind dark theme
- Recoil for state (ready to use)
- Mocked engine IPC (cefBridge)

## Scripts
- `npm i`
- `npm run dev`

## Build for CEF
`npm run build` → `dist/` → copy into your C++ app resources folder, e.g. `rc/ui/` or `bin/ui/`.