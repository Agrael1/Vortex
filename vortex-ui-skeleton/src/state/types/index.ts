export type ProjectPath = string | null;

export type ProjectMeta = {
  name: string;
  version: string;
  lastOpened?: string;
  template?: string | null;
};

export type ProjectSettings = {
  width: number;
  height: number;
  fps: number;
  colorSpace: string;
};

export type GraphNodeSnapshot = {
  id: string;
  type: string;
  label?: string;
  position: { x: number; y: number };
  ptr?: number;
  props?: Record<string, any>;
};

export type GraphEdgeSnapshot = {
  id: string;
  source: string;
  target: string;
  animated?: boolean;
};

export type GraphSnapshot = {
  nodes: GraphNodeSnapshot[];
  edges: GraphEdgeSnapshot[];
};

export type ProjectSnapshot = {
  path: ProjectPath;
  meta: ProjectMeta;
  settings: ProjectSettings;
  graph: GraphSnapshot;
  updatedAt?: string;
};

export type ConsoleEntry = {
  id: string;
  level: 'info' | 'warn' | 'error';
  message: string;
  time: number;
  scope?: string | null;
};
