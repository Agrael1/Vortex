import type { ProjectMeta, ProjectSettings } from '@state/types';

export type CommandKind =
  | 'graph.node.create'
  | 'graph.node.remove'
  | 'graph.node.position'
  | 'graph.node.label'
  | 'graph.node.props'
  | 'graph.edge.connect'
  | 'graph.edge.disconnect'
  | 'project.settings'
  | 'project.meta';

export type NodeHandle = {
  ptr?: number | null;
  id?: string | null;
};

export type NodeSlotHandle = NodeHandle & {
  slot?: number | null;
};

export type CommandEnvelope<Kind extends CommandKind, Payload> = {
  id?: string;
  kind: Kind;
  timestamp?: string;
  payload: Payload;
};

export type GraphNodeCreateCommand = CommandEnvelope<
  'graph.node.create',
  {
    type: string;
    label?: string;
    position?: { x: number; y: number };
    props?: Record<string, unknown>;
    clientNodeId?: string;
  }
>;

export type GraphNodeRemoveCommand = CommandEnvelope<'graph.node.remove', { target: NodeHandle }>;

export type GraphNodePositionCommand = CommandEnvelope<
  'graph.node.position',
  { target: NodeHandle; position: { x: number; y: number } }
>;

export type GraphNodeLabelCommand = CommandEnvelope<'graph.node.label', { target: NodeHandle; label: string }>;

export type GraphNodePropsCommand = CommandEnvelope<'graph.node.props', {
  target: NodeHandle;
  props: Record<string, unknown>;
}>;

export type GraphEdgeConnectCommand = CommandEnvelope<
  'graph.edge.connect',
  { source: NodeSlotHandle; target: NodeSlotHandle; edgeId?: string }
>;

export type GraphEdgeDisconnectCommand = CommandEnvelope<
  'graph.edge.disconnect',
  { edgeId?: string; source?: NodeSlotHandle; target?: NodeSlotHandle }
>;

export type ProjectSettingsCommand = CommandEnvelope<'project.settings', { settings: ProjectSettings }>;

export type ProjectMetaCommand = CommandEnvelope<'project.meta', { meta: Partial<ProjectMeta> }>;

export type ProjectCommand =
  | GraphNodeCreateCommand
  | GraphNodeRemoveCommand
  | GraphNodePositionCommand
  | GraphNodeLabelCommand
  | GraphNodePropsCommand
  | GraphEdgeConnectCommand
  | GraphEdgeDisconnectCommand
  | ProjectSettingsCommand
  | ProjectMetaCommand;

export type ProjectCommandBatch = {
  projectPath: string | null;
  baseRevision?: string | null;
  source?: string | null;
  issuedAt?: string;
  commands: ProjectCommand[];
};

export type FinalizedProjectCommand = ProjectCommand & { id: string; timestamp: string };

export type PreparedCommandBatch = Omit<ProjectCommandBatch, 'issuedAt' | 'commands' | 'projectPath'> & {
  projectPath: string;
  issuedAt: string;
  commands: FinalizedProjectCommand[];
};

const isoNow = () => new Date().toISOString();

const randomId = () =>
  typeof crypto !== 'undefined' && typeof crypto.randomUUID === 'function'
    ? crypto.randomUUID()
    : `cmd-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;

const cleanString = (value: string | null | undefined): string | undefined => {
  if (typeof value !== 'string') {
    return undefined;
  }
  const trimmed = value.trim();
  return trimmed.length ? trimmed : undefined;
};

const normalizeTimestamp = (value: string | null | undefined): string => {
  if (typeof value === 'string') {
    const trimmed = value.trim();
    if (trimmed.length && !Number.isNaN(Date.parse(trimmed))) {
      return trimmed;
    }
  }
  return isoNow();
};

export const normalizeNodeHandle = (handle: NodeHandle | null | undefined): NodeHandle | null => {
  if (!handle) {
    return null;
  }

  const ptr = Number(handle.ptr);
  const hasPtr = Number.isFinite(ptr) && ptr > 0;
  const id = cleanString(handle.id ?? undefined);

  if (!hasPtr && !id) {
    return null;
  }

  const normalized: NodeHandle = {};
  if (hasPtr) {
    normalized.ptr = ptr;
  }
  if (id) {
    normalized.id = id;
  }
  return normalized;
};

export const normalizeNodeSlotHandle = (handle: NodeSlotHandle | null | undefined): NodeSlotHandle | null => {
  const normalized = normalizeNodeHandle(handle);
  if (!normalized) {
    return null;
  }

  const slot = Number(handle?.slot);
  if (Number.isFinite(slot)) {
    return { ...normalized, slot };
  }
  return normalized;
};

export const prepareCommandBatch = (batch: ProjectCommandBatch): PreparedCommandBatch => {
  const projectPath = cleanString(batch.projectPath);
  if (!projectPath) {
    throw new Error('projectPath is required to submit commands');
  }
  if (!Array.isArray(batch.commands) || batch.commands.length === 0) {
    throw new Error('commands must contain at least one command');
  }

  const issuedAt = normalizeTimestamp(batch.issuedAt);

  const commands: FinalizedProjectCommand[] = batch.commands.map((command) => {
    const id = cleanString(command.id) ?? randomId();
    const timestamp = normalizeTimestamp(command.timestamp ?? issuedAt);
    return { ...command, id, timestamp } as FinalizedProjectCommand;
  });

  return {
    projectPath,
    baseRevision: cleanString(batch.baseRevision ?? undefined) ?? null,
    source: cleanString(batch.source ?? undefined) ?? null,
    issuedAt,
    commands,
  };
};

export const serializeCommandBatch = (batch: PreparedCommandBatch): string => JSON.stringify(batch);
