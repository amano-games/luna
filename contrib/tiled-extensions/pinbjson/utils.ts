import path from "pathe";

export function getAssetPath(value: string) {
  const parts = value.split(path.sep); // Split using OS-specific separator
  const srcIndex = parts.indexOf("assets");
  const res = srcIndex !== -1 ? path.join(...parts.slice(srcIndex)) : value;
  return res as string;
}
