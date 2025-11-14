import { useEffect, useState } from 'react';
import { Vortex } from '@/bridge/vortex';
import { PropertyEditor, PropertySpec } from '@/app/components/PropertyEditor';

type Props = { selectedPtr: number | null };

interface PropertySchema {
  properties: PropertySpec[];
}

export function InspectorPanel({ selectedPtr }: Props) {
  const [properties, setProperties] = useState<PropertySpec[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    let cancelled = false;

    if (!selectedPtr) {
      setProperties([]);
      setError(null);
      return;
    }

    setLoading(true);
    setError(null);

    (async () => {
      try {
        const raw = await Vortex.getNodeProperties(selectedPtr);
        
        if (cancelled) return;

        let parsedProps: PropertySchema;
        
        if (typeof raw === 'string') {
          try {
            parsedProps = JSON.parse(raw);
          } catch {
            // If parsing fails, treat it as raw text and show error
            setError(`Invalid JSON response: ${raw}`);
            setProperties([]);
            return;
          }
        } else {
          parsedProps = raw as PropertySchema;
        }

        if (parsedProps && parsedProps.properties && Array.isArray(parsedProps.properties)) {
          setProperties(parsedProps.properties);
        } else {
          setError('Invalid properties format received from engine');
          setProperties([]);
        }
      } catch (e: any) {
        if (!cancelled) {
          setError(`Failed to load properties: ${e?.message ?? e}`);
          setProperties([]);
        }
      } finally {
        if (!cancelled) {
          setLoading(false);
        }
      }
    })();

    return () => { cancelled = true; };
  }, [selectedPtr]);

  if (!selectedPtr) {
    return (
      <div className="h-full flex items-center justify-center text-gray-500">
        <div className="text-center">
          <div className="text-4xl mb-2">⚙️</div>
          <div>Select a node to view properties</div>
        </div>
      </div>
    );
  }

  if (loading) {
    return (
      <div className="h-full flex items-center justify-center text-gray-500">
        <div className="text-center">
          <div className="animate-spin text-2xl mb-2">⚙️</div>
          <div>Loading properties...</div>
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="p-4">
        <div className="text-red-400 text-sm mb-3">
          <strong>Error:</strong> {error}
        </div>
        <details className="text-xs text-gray-500">
          <summary className="cursor-pointer hover:text-gray-400">
            Debug Info
          </summary>
          <pre className="mt-2 p-2 bg-gray-800 rounded text-xs overflow-auto">
            Selected Node: #{selectedPtr}
          </pre>
        </details>
      </div>
    );
  }

  return (
    <div className="h-full overflow-auto">
      <PropertyEditor
        nodePtr={selectedPtr}
        properties={properties}
        className="h-full"
      />
    </div>
  );
}
