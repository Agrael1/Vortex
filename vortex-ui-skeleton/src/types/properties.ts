export type PropertyEnumOption = {
  label: string;
  value: any;
};

export interface PropertySpec {
  name: string;
  label?: string;
  type: 'int' | 'float' | 'bool' | 'string' | 'enum' | 'color' | 'vec2' | 'vec3' | 'vec4';
  index: number;
  default?: any;
  min?: number;
  max?: number;
  step?: number;
  enum?: PropertyEnumOption[] | null;
  value?: any;
}

export type NodePropertyCache = Record<string, PropertySpec[]>;
