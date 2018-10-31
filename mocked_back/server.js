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

//-----------------------------------------


//let questionsID = [0,1,2,3,4,5,6,7,8,9,10,11,12,13];
let questionsID = [14,14,14,14,14,14,14,14,14,14,14,14,14];

function shuffle (array) {
  for (var i = array.length - 1; i > 0; i--) {
      var j = Math.floor(Math.random() * (i + 1));
      var temp = array[i];
      array[i] = array[j];
      array[j] = temp;
  }
  return array;
}

let cpt= 0
//------------------------------------------------------
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
	if (cpt < 5){
		questionsID = shuffle(questionsID)
		nextQuestion = questionsID.pop()  
  		console.log("next question : "+ nextQuestion)
		res.json({'nextQuestionId' : nextQuestion})
	} else {
		console.log("next question : "+ "stop")
		res.json({'nextQuestionId' : "stop"})
		}
	
	cpt = cpt+1
	
})

app.listen(port);


console.log('todo list RESTful API server started on: ' + port);