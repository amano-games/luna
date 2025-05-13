import path from "pathe";

export function getImgPath(img: string) {
  const parts = img.split(path.sep); // Split using OS-specific separator
  const srcIndex = parts.indexOf("assets");
  const res = srcIndex !== -1 ? path.join(...parts.slice(srcIndex)) : img;
  return res as string;
}
