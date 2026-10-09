import { createApp } from "./app.js";
import { findScheduler } from "./scheduler.js";

const port = Number(process.env.PORT ?? 5174);

createApp().listen(port, async () => {
  console.log(`API listening on http://localhost:${port}`);
  const scheduler = await findScheduler();
  console.log(
    scheduler ? `Using scheduler: ${scheduler.label}` : "WARNING: scheduler not built yet",
  );
});
