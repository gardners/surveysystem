import React from 'react';
import ReactDOM from 'react-dom';
import registerServiceWorker from './registerServiceWorker';
import 'bootstrap/dist/css/bootstrap.css';

import Navbar from "./Navbar";
import surveyJSON from "./ressources/dataTest"
import FlexibleSurvey from "./FlexibleSurvey";



//rendering the Navbar
ReactDOM.render(
    <Navbar />,
    document.getElementById("header"));

//rendering the flexible survey
ReactDOM.render(
    <FlexibleSurvey json={surveyJSON} />,
    document.getElementById("surveyContainer"));



registerServiceWorker();
