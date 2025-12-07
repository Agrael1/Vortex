export function legacyBridgeRemoved(): never {
	throw new Error('Legacy bridge has been removed. Use EngineBridge (src/app/services/ipc/cefBridge.ts).');
}
