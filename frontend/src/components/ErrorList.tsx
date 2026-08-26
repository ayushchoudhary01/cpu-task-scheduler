import { AlertCircle } from "lucide-react";

// The server reports every problem it found rather than just the first, so this
// always renders a list.
export default function ErrorList({ errors }: { errors: string[] }) {
  if (errors.length === 0) return null;

  return (
    <div className="rounded-lg border border-red-900 bg-red-950/40 p-4 text-sm text-red-200">
      <p className="flex items-center gap-2 font-medium">
        <AlertCircle size={16} /> That did not work
      </p>
      <ul className="mt-2 list-inside list-disc space-y-1 text-red-300/90">
        {errors.map((message, index) => (
          <li key={index}>{message}</li>
        ))}
      </ul>
    </div>
  );
}
