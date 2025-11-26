import { useCallback, useEffect, useState } from 'react';
import type { ChangeEvent } from 'react';
import { useGraphCommands } from '@state/hooks/useGraphCommands';

export interface PropertySpec {
  name: string;
  label?: string;
  type: 'int' | 'float' | 'bool' | 'string' | 'enum' | 'color' | 'vec2' | 'vec3' | 'vec4';
  index: number;
  default?: any;
  min?: number;
  max?: number;
  step?: number;
  enum?: { label: string; value: any }[] | null;
  value?: any;
}

interface PropertyEditorProps {
  nodePtr: number;
  properties: PropertySpec[];
  className?: string;
  liveValues?: Record<string, any> | null;
}

export function PropertyEditor({ nodePtr, properties, className = '', liveValues }: PropertyEditorProps) {
  const [localValues, setLocalValues] = useState<Record<string, any>>({});
  const { updateNodeProps } = useGraphCommands();

  useEffect(() => {
    setLocalValues({});
  }, [nodePtr, properties]);

  useEffect(() => {
    if (!liveValues) return;
    setLocalValues((prev) => {
      let changed = false;
      const next = { ...prev };
      for (const prop of properties) {
        if (!prop || typeof prop.name !== 'string') continue;
        if (!Object.prototype.hasOwnProperty.call(liveValues, prop.name)) continue;
        const incoming = (liveValues as Record<string, any>)[prop.name];
        if (Object.is(next[prop.name], incoming)) continue;
        next[prop.name] = incoming;
        changed = true;
      }
      return changed ? next : prev;
    });
  }, [liveValues, properties]);

  const handleValueChange = useCallback(
    async (prop: PropertySpec, value: any) => {
      try {
        // Update local state immediately for responsiveness
        setLocalValues((prev) => ({ ...prev, [prop.name]: value }));

        await updateNodeProps(nodePtr, { [prop.name]: value });
      } catch (error) {
        console.error(`Failed to update property ${prop.name}:`, error);
        // Revert local value on error
        setLocalValues((prev) => {
          const newState = { ...prev };
          delete newState[prop.name];
          return newState;
        });
      }
    },
    [nodePtr, updateNodeProps],
  );

  const getCurrentValue = (prop: PropertySpec) => {
    if (localValues[prop.name] !== undefined) {
      return localValues[prop.name];
    }
    if (liveValues && Object.prototype.hasOwnProperty.call(liveValues, prop.name)) {
      return (liveValues as Record<string, any>)[prop.name];
    }
    return prop.value ?? prop.default;
  };

  const renderPropertyInput = (prop: PropertySpec) => {
    const currentValue = getCurrentValue(prop);
    const commonProps = {
      key: prop.name,
      onChange: (value: any) => handleValueChange(prop, value),
    };

    switch (prop.type) {
      case 'bool':
        return <BooleanInput {...commonProps} value={currentValue} label={prop.label || prop.name} />;

      case 'int':
        return (
          <NumberInput
            {...commonProps}
            value={currentValue}
            label={prop.label || prop.name}
            min={prop.min}
            max={prop.max}
            step={prop.step || 1}
            isFloat={false}
          />
        );

      case 'float':
        return (
          <NumberInput
            {...commonProps}
            value={currentValue}
            label={prop.label || prop.name}
            min={prop.min}
            max={prop.max}
            step={prop.step || 0.1}
            isFloat={true}
          />
        );

      case 'string':
        return <StringInput {...commonProps} value={currentValue} label={prop.label || prop.name} />;

      case 'enum':
        return <EnumInput {...commonProps} value={currentValue} label={prop.label || prop.name} options={prop.enum || []} />;

      case 'color':
        return <ColorInput {...commonProps} value={currentValue} label={prop.label || prop.name} />;

      case 'vec2':
      case 'vec3':
      case 'vec4':
        return (
          <VectorInput
            {...commonProps}
            value={currentValue}
            label={prop.label || prop.name}
            dimensions={prop.type === 'vec2' ? 2 : prop.type === 'vec3' ? 3 : 4}
          />
        );

      default:
        return (
          <div key={prop.name} className="text-gray-500 text-sm">
            Unsupported type: {prop.type}
          </div>
        );
    }
  };

  if (properties.length === 0) {
    return <div className={`p-4 text-center text-gray-500 ${className}`}>No properties available</div>;
  }

  return (
    <div className={`p-4 space-y-4 ${className}`}>
      <div className="text-sm font-semibold text-gray-300 mb-3">Node Properties #{nodePtr}</div>

      {properties.map(renderPropertyInput)}
    </div>
  );
}

// Individual input components
interface InputProps {
  value: any;
  label: string;
  onChange: (value: any) => void;
}

function BooleanInput({ value, label, onChange }: InputProps) {
  return (
    <div className="flex items-center justify-between">
      <label className="text-sm text-gray-300">{label}</label>
      <input
        type="checkbox"
        checked={Boolean(value)}
        onChange={(e) => onChange(e.target.checked)}
        className="w-4 h-4 text-blue-600 bg-gray-700 border-gray-600 rounded focus:ring-blue-500"
      />
    </div>
  );
}

interface NumberInputProps extends InputProps {
  min?: number;
  max?: number;
  step?: number;
  isFloat?: boolean;
}

function NumberInput({ value, label, onChange, min, max, step, isFloat }: NumberInputProps) {
  const handleChange = (e: ChangeEvent<HTMLInputElement>) => {
    const newValue = isFloat ? parseFloat(e.target.value) : parseInt(e.target.value);
    if (!isNaN(newValue)) {
      onChange(newValue);
    }
  };

  return (
    <div className="space-y-1">
      <label className="text-sm text-gray-300">{label}</label>
      <input
        type="number"
        value={value || 0}
        onChange={handleChange}
        min={min}
        max={max}
        step={step}
        className="w-full px-3 py-1 text-sm bg-gray-800 border border-gray-600 rounded focus:border-blue-500 focus:outline-none text-white"
      />
    </div>
  );
}

function StringInput({ value, label, onChange }: InputProps) {
  return (
    <div className="space-y-1">
      <label className="text-sm text-gray-300">{label}</label>
      <input
        type="text"
        value={value || ''}
        onChange={(e) => onChange(e.target.value)}
        className="w-full px-3 py-1 text-sm bg-gray-800 border border-gray-600 rounded focus:border-blue-500 focus:outline-none text-white"
      />
    </div>
  );
}

interface EnumInputProps extends InputProps {
  options: { label: string; value: any }[];
}

function EnumInput({ value, label, onChange, options }: EnumInputProps) {
  return (
    <div className="space-y-1">
      <label className="text-sm text-gray-300">{label}</label>
      <select
        value={value || ''}
        onChange={(e) => onChange(e.target.value)}
        className="w-full px-3 py-1 text-sm bg-gray-800 border border-gray-600 rounded focus:border-blue-500 focus:outline-none text-white"
      >
        {options.map((option, index) => (
          <option key={index} value={option.value}>
            {option.label}
          </option>
        ))}
      </select>
    </div>
  );
}

function ColorInput({ value, label, onChange }: InputProps) {
  return (
    <div className="space-y-1">
      <label className="text-sm text-gray-300">{label}</label>
      <div className="flex gap-2">
        <input
          type="color"
          value={value || '#ffffff'}
          onChange={(e) => onChange(e.target.value)}
          className="w-12 h-8 rounded border border-gray-600"
        />
        <input
          type="text"
          value={value || '#ffffff'}
          onChange={(e) => onChange(e.target.value)}
          className="flex-1 px-3 py-1 text-sm bg-gray-800 border border-gray-600 rounded focus:border-blue-500 focus:outline-none text-white"
          placeholder="#ffffff"
        />
      </div>
    </div>
  );
}

interface VectorInputProps extends InputProps {
  dimensions: number;
}

function VectorInput({ value, label, onChange, dimensions }: VectorInputProps) {
  const vectorValue = Array.isArray(value) ? value : new Array(dimensions).fill(0);
  const labels = ['X', 'Y', 'Z', 'W'];

  const handleComponentChange = (index: number, newValue: string) => {
    const parsed = parseFloat(newValue);
    if (!isNaN(parsed)) {
      const newVector = [...vectorValue];
      newVector[index] = parsed;
      onChange(newVector);
    }
  };

  return (
    <div className="space-y-1">
      <label className="text-sm text-gray-300">{label}</label>
      <div className="grid grid-cols-2 gap-2">
        {Array.from({ length: dimensions }, (_, i) => (
          <div key={i} className="flex items-center gap-1">
            <span className="text-xs text-gray-400 w-3">{labels[i]}</span>
            <input
              type="number"
              step="0.1"
              value={vectorValue[i] || 0}
              onChange={(e) => handleComponentChange(i, e.target.value)}
              className="flex-1 px-2 py-1 text-xs bg-gray-800 border border-gray-600 rounded focus:border-blue-500 focus:outline-none text-white"
            />
          </div>
        ))}
      </div>
    </div>
  );
}
