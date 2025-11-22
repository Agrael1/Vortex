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
- `npm run typecheck`
- `npm run lint`
- `npm run test`
- `npm run format`
- `npm run check`

## Build for CEF

`npm run build` → `dist/` → copy into your C++ app resources folder, e.g. `rc/ui/` or `bin/ui/`.

## Testing

- `npm run test` — однократный прогон Vitest + jsdom
- `npm run test:watch` — режим разработчика с live-перезапуском
