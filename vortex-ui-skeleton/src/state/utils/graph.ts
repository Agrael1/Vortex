import type { GraphSnapshot } from '@state/types';

const DEFAULT_NODE_POSITION = { x: 120, y: 120 };
const NODE_SPACING = 40;

export const DEFAULT_GRAPH_NODE_POSITION = { ...DEFAULT_NODE_POSITION };

export function computeNextNodePosition(graph?: GraphSnapshot | null) {
  if (!graph?.nodes?.length) {
    return { ...DEFAULT_GRAPH_NODE_POSITION };
  }

  const last = graph.nodes[graph.nodes.length - 1];
  const baseX = Number.isFinite(last?.position?.x) ? last.position.x : DEFAULT_NODE_POSITION.x;
  const baseY = Number.isFinite(last?.position?.y) ? last.position.y : DEFAULT_NODE_POSITION.y;

  return { x: baseX + NODE_SPACING, y: baseY + NODE_SPACING };
}
