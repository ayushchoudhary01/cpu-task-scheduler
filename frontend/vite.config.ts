import tailwindcss from "@tailwindcss/vite";
import react from "@vitejs/plugin-react";
import { defineConfig } from "vite";

// The UI runs on 5173 and the API on 5174. Anything the browser asks for under
// /api is forwarded to the API server, so the front end can use plain relative
// URLs and there is no CORS to configure.
export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    port: 5173,
    // Fail rather than quietly moving to another port. Drifting onto 5174
    // would collide with the API server and leave requests hanging with no
    // obvious cause.
    strictPort: true,
    proxy: {
      "/api": "http://127.0.0.1:5174",
    },
  },
  build: {
    outDir: "dist",
  },
});
