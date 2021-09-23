import { Component, OnInit } from '@angular/core';

@Component({
  selector: 'app-page2',
  templateUrl: './page2.page.html',
  styleUrls: ['./page2.page.scss'],
})



export class Page2Page implements OnInit {

  img:string;

  constructor() { }

  ngOnInit() {
  }

  imagenes = [
    'assets/frutas.jpeg',
    'assets/lácteos.jpg',
    'assets/plantas.jpg',
    'assets/semillas.jpg',
    'assets/verduras.jpg',
    'assets/nuevo.jpg',

  ]

}
