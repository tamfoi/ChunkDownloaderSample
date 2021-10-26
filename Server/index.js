const express = require("express");
const app = express();
const port = 8080;

express.static.mime.define({ "application/octet-stream": ["pak"] });
app.use(express.static("public"));

app.listen(port, () => {
  console.log(`Example app listening at http://localhost:${port}`);
});
