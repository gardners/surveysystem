var express = require('express'),
  app = express(),
  port = process.env.PORT || 4000,
  bodyParser = require('body-parser');
 
const survey = require('./testSurvey.json')

app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

app.use(function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});

app.get('/survey/:surveyid/newSession', function(req,res){
	console.log("GET /survey/id")
	res.json(survey)
})

app.post('/addAnswer/session/:sessionid', function (req, res) {
  console.log(req.body)
  setTimeout(function(){
    res.send('POST request received');
}, 2000);

});

app.get('/nextQuestion/session/:id', function(req,res){
	res.json({'nextQuestion' : 6})
})

app.listen(port);


console.log('todo list RESTful API server started on: ' + port);