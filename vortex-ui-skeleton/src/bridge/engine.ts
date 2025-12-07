import { engine as cefEngine } from '@/app/services/ipc/cefBridge';

/**
 * Legacy compatibility shim: prefer importing from '@/app/services/ipc/cefBridge'.
 * This file now simply re-exports the modern engine instance so older code keeps working.
 */
export const engine = cefEngine;
