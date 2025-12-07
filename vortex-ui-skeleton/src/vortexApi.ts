export const Vortex = new Proxy(
  {},
  {
    get() {
      throw new Error(
        '[Legacy Vortex API] Direct calls are no longer supported. Use EngineBridge (src/app/services/ipc/cefBridge.ts).',
      );
    },
  },
);
