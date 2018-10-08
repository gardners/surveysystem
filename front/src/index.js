import React from 'react';
import ReactDOM from 'react-dom';
import registerServiceWorker from './registerServiceWorker';
import 'bootstrap/dist/css/bootstrap.css';

import Navbar from "./Navbar";
import FlexibleSurvey from "./FlexibleSurvey";

import { BrowserRouter as Router, Route } from 'react-router-dom';



//rendering the routes
ReactDOM.render(
    <Navbar />,
    document.getElementById("header"));

ReactDOM.render(
    <Router>
        <div>
            <Route path="/survey/:id" component={FlexibleSurvey} />
        </div>
    </Router>,
    document.getElementById('app')
)



registerServiceWorker();
